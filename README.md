<h1 style="text-align: center">HTML Macro</h1>
<p style="text-align: center">by: Hurkus</p>


HTML Macro is a CLI tool for generating static website HTML files.


# Usage


Pass the path to the main HTML file as an argument:<br/>
`./html-macro [options] <path>`

Example, parse and evaluate `main.html` and pipe into output file `index.html`: <br/>
`./html-macro 'main.html' -o 'index.html'`


## Options

* `--help`, `-h` print help.
* `--version`, `-v` print version.
* `--output <path>`, `-o <path>` pipe result into a file instead of stdout.




# Macros

* Regular tag
	* `<attr>="<value>"` copy attribute and interpolate value.
	* `CALL="<str>"` call macro inside the tag.
	* `IF="<expr>"` delete tag if `<expr>` evaluates to `false`.
	* `INTERPOLATE="<expr>"` interpolate plain text of all child elements if expression evaluates to `true`.

* Plain text
	* Expressions inside curly brackets `<p>text {expr} text</p>` are interpolated.

* SET
	* Attributes are variable names and their values. Values can be expressions: `x="{y + 1}"`
	<!-- * `IF="<expr>"` assign variables only when expression evaluates to `true`. -->

* IF, ELSE-IF, ELSE
	* `TRUE="<expr>"` execute branch if expression evaluates to `true`.
	* `FALSE="<expr>"` execute branch if expression evaluates to `false`.
	* `INTERPOLATE="<expr>"` interpolate plain text of all child elements if expression evaluates to `true`.

* FOR
	* First (optional) attribute sets a variable with the same name (same as `<SET/>`).
	* Second attribute `TRUE="<expr>"` or `FALSE="<expr>"` evaluates for each loop.
	* Third (optional) attribute sets a variable after each iteration.
	* `IF="<expr>"` execute only when `<expr>` evaluates to `true`.
	* `INTERPOLATE="<expr>"` interpolate plain text of all child elements if expression evaluates to `true`.

* WHILE
	* `TRUE="<expr>"` or `FALSE="<expr>"` are evaluated on each iteration. Loop ends when condition fails.
	The user must prevent infinite loops by setting the conditional variable inside the loop.
	* `IF="<expr>"` execute only when `<expr>` evaluates to `true`.
	* `INTERPOLATE="<expr>"` interpolate plain text of all child elements if expression evaluates to `true`.

* SET-ATTR
	* Set attributes of the same name on the parent node. Values are interpolated for any expressions.<br/>
	ex: `<SET-ATTR width="{100+1}"/>` sets variable `width` to `101` on the parent node.
	* `IF="<expr>"` execute only when `<expr>` evaluates to `true`.
	
* GET-ATTR
	* Set variables with the same name as the attributes on the macro node to the values of attributes on the parent node. Names of parent attributes are listed in the macro node attribute value.<br/>
	ex: `<GET-ATTR x="src"/>` sets variable `x` to the value of the attribute `src="..."` of the parent node.
	* `IF="<expr>"` execute only when `<expr>` evaluates to `true`.
	
* DEL-ATTR
	* Delete attributes of the same name on the parent node. Values of attributes on the macro node are ignored.
	* `IF="<expr>"` execute only when `<expr>` evaluates to `true`.
	
* SET-TAG
	* `NAME="<value>"` set tag of the parent node to the interpolated value.
	* `IF="<expr>"` execute only when `<expr>` evaluates to `true`.
	
* GET-TAG
	* Set variables named by attribute names to the value of the parent node tag.<br/>
	ex: `<div><GET-TAG x=""/></div>` sets variable `x` to `div`.
	* `IF="<expr>"` execute only when `<expr>` evaluates to `true`.

* MACRO
	* `NAME="<str>"` name of the macro. This name is used by `<CALL/>` to invoke the macro.
	* All `<MACRO></MACRO>` nodes are extracted from the main node hierarchy after parsing the file.
	
* CALL
	* `NAME="<str>"` name of macro to invoke.
	* `IF="<expr>"` execute only when `<expr>` evaluates to `true`.

* INCLUDE
	* `SRC="<str>"` path to new HTML file to include into the current file.
	* Macros from the new file replace existing macros of the same name.
	* `IF="<expr>"` execute only when `<expr>` evaluates to `true`.
	
* SHELL
	* `VARS="<str>,<str>,..."` list of variables that are inserted into the environmental variables of the spawned shell.
	* `STDOUT="VOID"` destroy stdout.
	* `STDOUT="HTML"` parse stdout as an included file.
	* `STDOUT="TEXT"` include stdout as raw text.
	* `STDOUT="<str>"` set variable named in the attribute value to stdout.
	* `IF="<expr>"` execute only when `<expr>` evaluates to `true`.
	
* INFO, WARN, ERROR
	* Print body of the node as a message to stderr.
	* `IF="<expr>"` execute only when `<expr>` evaluates to `true`.
	* `INTERPOLATE="<expr>"` interpolate plain text of all child elements if expression evaluates to `true`.




# Expressions

Expressions inside interpolated strings or attribute values can evaluate to 3 types: `long`, `double` or `string`.
Syntax is intuitive and predictable: `(1.5 + x) * 3`. Operator `!` converts the operand into a bool (`0`) and then inverts it.
Strings are literals inside quotes `"` or `'`. Functions are also obvious: `len('abc') == 3` evaluates to `1` (booleans are numbers).


## Constants

Constants are just pre-initialised variables (and can be overwritten lol).

* `false` = `0`
* `true` = `1`
* `pi` = `3.14`


## Functions

* `int(x)` convert `x` into an integer (long).
* `float(x)` convert `x` into a number (double).
* `str(x)` convert `x` into a string.
* `len(x)` length of string `x` or absolute value if its a number.
* `abs(x)` absolute value of `x` or length of string.



# Example

Some input file
```html
<div>
	<SET x="'box'" y="5 > 3" />
	<SET-ATTR IF="y" class="{x}" />
	<p><SHELL>cat 'text.txt'</SHELL></p>
</div>
```

results in

```html
<div class="box">
	<p>hello world</p>
</div>
```
