# Bracketext

## Text macros language

Bracketext is a macro language using only 4 symbols [ | ] and \' acting on the text in which the macros are placed when they are interpreted.

These macros must be written in scripting languages like PowerShell. Subsequently, another scripting language may be added to avoid Powershell automation, which causes versioning problems with .NET Core 2.2.

We use a scripting language to create macros that when inserted into any text will transform it into HTML for example.

Let's give an example. If we declare in the file ``macro.txt``:

    # <<<<<<
    # ||||||2|h1|/h1
    function Out-H1($param, $arg) {
    $p=[string[][]]$param;
    $a=[string[]]$arg;
    $x=$a[0];
    "<h1>$x</h1>"
    }
    # >>>>>>

In the text, we can write:

bla [h1] Title [/h1] bla

This text will be transformed into ``HTML`` as 

    bla <h1>  Title </h1> bla

Inverse quote inhibits the effect of other symbols [ | ] in the text when it precedes them.

For example \`[Example\`] will produce the text [example]

The general syntax of a macro is as follows... 

[MacroName1|param_1_1|param_1_2| ... |param_1_N1] Text1 [MacroName2|param_2_1|param_2_2| ... |param_2_N2] Text2 [MacroName3|param_3_1|param_3_2| ... |param_3_N3] ...

We suppose that MacroName1, MacroName2 and MacroName3 are linked in the macrot.txt file:

    # <<<<<<
    # ||||||3|MacroName1|MacroName2|MacroName3

The number 3 indicates that MacroName2 can be repeated between MacroName1 and MacroName3.

Such a macro is represented by Bracketext as a tree and transformed into a call to a PowerShell function:

 MacroName1
      |                        \        \           \
    param1                    param2   param3     arguments
      |        |    \          /|\      /|\       /   |   \
    param_1_1 ... param_1_N    ...      ...    text1 text1 text3

So MacroName2 and MacroName3 disappear, they have a delimiter role to the macro MacroName1, the text placed between the tags is found as an argument of MacroName1 and the possible parameters of MacroName1, MacroName2 and MacroName3 are grouped together.

This tree is then transformed into a call to the MacroName1 function whose parameters are strings:

function Function1($param, $arg)

$param is of type string[][] 
$arg is of type string[]

Let's give a simple example:

   bla1 [color|blue] the world is blue [color-|X] bla2

in the file macro.txt:

    # <<<<<<
    # ||||||2|color|color-
    function Color($param, $arg) {
    $p=[string[][]]$param;
    $a=[string[]]$arg;
	$col=$p[0][0];
	$x=$p[1][0];
    $text=$a[0];
	"<span style=""color: $col;"">$x$text</span>"
    }
    # >>>>>>

	The result:

<span style="color: blue;">X the world is blue </span>

The representation in memory:

    color    
      |                 \                   \
    param block 1    param block 2        arguments
      |                   |                  |
    blue                 X            the world is blue 


## Compilation

This first version works. 

To compile the project, you need ``Visual Studio`` and install the ``NugetMicrosoft.PowerShell.5.ReferenceAssemblies`` Nuget.

## To Do

There are still some improvements to be made to Bracketext's command line call options.

It also lacks predefined macros (to declare global variables, ...)

It lacks macro files (other than those of the ``BBcode``) some converters to ``Markdown`` for example.