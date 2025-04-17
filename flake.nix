{
  description = "HTML Macro - flake";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { nixpkgs, ... }:
    let
      systems = [ "x86_64-linux" ];
      eachSystem = f: nixpkgs.lib.genAttrs systems (
        system: f {
          inherit system;
          pkgs = nixpkgs.legacyPackages.${system};
        }
      );
    in {

      # ,----------------------------------------------------------------------
      # | Packages
      # '----------------------------------------------------------------------

      packages = eachSystem ({ pkgs, ... }: rec {
        # Point to html-macro by default
        default = html-macro;

        # HTML macro derivation
        html-macro = pkgs.stdenv.mkDerivation {
          name = "html-macro";
          src = ./.;
          buildInputs = with pkgs; [
            perl
          ];

          bash = pkgs.bash;

          buildPhase = ''
            patchShebangs makefile-targets.sh
            substituteInPlace makefile --replace-warn "/usr/bin/env bash" "$bash/bin/bash"
            make
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp bin/html-macro $out/bin/html-macro
          '';
        };
      });

      # ,----------------------------------------------------------------------
      # | DevShells
      # '----------------------------------------------------------------------

      devShells = eachSystem (
        { pkgs, ... }:
        {
          default = pkgs.mkShell {
            packages = with pkgs; [
              clang-tools # Language server
            ];
          };
        }
      );
    };
}
