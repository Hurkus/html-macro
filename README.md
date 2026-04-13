<h1 align="center">HTML Macro</h1>
<p align="center">Version 0.13</p>


**HTML-Macro** is a CLI tool for generating static website HTML files.

The main benefits of the tool are:
* Splitting large documents into smaller, more managable files.
* Generating table or grid entries based on templates.
* Easier generation of language variants of the document.

# Usage


Pass the path to the main HTML file as an argument:<br/>
`html-macro [options] [variables] <path>`

Example, parse and evaluate `src/main.html` and store output to file `web/index.html`: <br/>
`html-macro 'src/main.html' -o 'web/index.html'`


## Options

| Long option         | Short option | Description    |
| ------------------- | ------------ | -------------- |
| `--help`            | `-h`         | Print help.    |
| `--include <path>`  | `-i <path>`  | Add path to the list of paths that are searched when <br/>including files using a relative path with the `<INCLUDE>` element macro. |
| `--output <path>`   | `-o <path>`  | Write output directly to a file instead of stdout. |
| `--type <type>`     | `-t <type>`  | Force input file to be treated as a different file type, <br/>where `<type>` can be `html`, `css`, `js` or `txt`. |
| `--compress <type>` | `-c <type>`  | Compress output by removing unecessary spaces and other constructs. The `<type>` can be `none`, `html`, `css` or `all`. |
| `--nostdout`        | `-x`         | Discard any output except errors and warnings. |
| `--dependencies`    | `-d`         | Print paths from that reference other files (such as the `<INCLUDE>` macro). <br/>This is usefull when generating dependency files with [make](https://www.gnu.org/software/make/). |


# Documentation

The full documentation on all macros can be found in the [doc/documentation.html](./doc/documentation.html) file.

 	 	
# Compile

For release build:<br/>
`make -j32 config=release all`

To run all tests:<br/>
`make test`

