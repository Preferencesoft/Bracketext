# Bracketext

## Text macros language

The Bracketext software has been rewritten in C++ and no longer uses JavaScript but Lua for macros. However, I still need to complete the macros. For now, Bracketext only generates HTML, but it will be able to generate Latex, for example, without any problems. The disadvantage of Bracketext is that a text must be accompanied by the macro file in order to be converted, and it is not a standardized language like SGML, HTML, etc.

The choice of C++ version, C++98, guarantees compilation on a maximum number of platforms. 
In order to use the Lua 5.2 and 5.3 libraries, I allowed the compiler to handle long long integers. 
However, the project can be compiled without modification using a C++11 standard compiler.

TODO:

- declare macros that allow you to manage counters for automatic numbering.

- Create a generic “official” macro file that can be used to convert to different formats.
Create a macro file more specialized in HTML production (and Bootstrap, for example).

- more documentation and examples

Progress:

- All macro and example files have been moved to the macros directory.

- The `bbcode` macro file named `macros-bbcode-to-html.txt is located in the macros directory.

- The `macros.txt` macro command file (macros.txt) is becoming more complete, 
and the lists and tables have been checked. The tables have the main HTML5 attributes.
But I also don't want to get too close to HTML, or I'll have trouble converting to other languages.   

- I still need to create a .css style sheet file, mainly to avoid cluttering up the tags with style=... parameters.

- You can now declare global variables in the macros.txt file by placing them in a field of the form below.

- Macros can now use multiple intermediate tags, as in the following example:
[command] text1 [line] tex2 [col] text3 [col] text4 [col] [line]  [/command]
The tags [line] and [col] must not, of course, be used as command tags or as intermediate tags for other commands. 
But the difficulty that arises is being able to find the type of intermediate tag (in the example [col] or [line]) in the parameters and
 I will explain how to program using the index information in the association table.


    -- <<<<<<
    -- ||||||v|
    local counter=1
    local n=12334
    -- >>>>>>

## Software overview

Bracketext is a macro language using only 4 symbols [ | ] and \ acting on the text in which the macros are placed when they are interpreted.

These symbols are used to form tags. These are always in the form:

[tag_name] or [tag_name|param1|...|param_n].

A macro can be defined by a single tag or by a sequence of several tags (as before) in the form:

[begin] or [begin] ... [end]

or more generally:

[begin]       [t1]      [t2]       [tn]       [end]

The ti tags can be repeated; the parts of text placed between each tag are considered the arguments of the macro. If the macro consists of only one tag, it will have no arguments.

These macros must be written in scripting languages like **Lua**. To write macros, you need to use *Lua* programming language. 

We use a scripting language to create macros that when inserted into any text will transform it into HTML for example.

![Bracketext logo](https://github.com/Preferencesoft/Bracketext/blob/master/b1.png)

Let's give an example. If we declare in the file ``macros.txt``:

    -- <<<<<<
    -- ||||||2|h1|/h1
    function outU(pa, ar)
    local oList = {}
    table.insert(oList,'\1')
    table.insert(oList,'<h1>')
    table.insert(oList,'\4')
    if type(ar) == "table" and #ar > 0 and type(ar[1]) == "string" then
      table.insert(oList,ar[1])
    end
    table.insert(oList,'\1')
    table.insert(oList,'</h1>')
    table.insert(oList,'\4')
    return oList
    end
    -- >>>>>>

In the text, we can write:

bla [h1] Title [/h1] bla

This text will be transformed into ``HTML`` as 

    bla <h1>  Title </h1> bla

Inverse quote inhibits the effect of other symbols [ | ] in the text when it precedes them.

For example \[Example\] will produce the text [example]

The general syntax of a macro is as follows... 

[MacroName1|param_1_1|param_1_2| ... |param_1_N1] Text1 [MacroName2|param_2_1|param_2_2| ... |param_2_N2] Text2 [MacroName3|param_3_1|param_3_2| ... |param_3_N3] ...

We suppose that MacroName1, MacroName2 and MacroName3 are linked in the macros.txt file:

    // <<<<<<
    // ||||||3|MacroName1|MacroName2|MacroName3

The number 3 indicates that MacroName2 can be repeated between MacroName1 and MacroName3.

For example:

[MacroName1|param_1_1| ... |param_1_N1] text1 [MacroName2|param_2_1| ... |param_2_N2] text2 [MacroName3|param_3_1| ... |param_3_N3]

Such a macro is represented by Bracketext as a tree and transformed into a call to a PowerShell function:

    MacroName1
        |                        \        \           \
      param1                    param2   param3     arguments
        |      |    \            /|\      /|\       /   |   \
    param_1_1 ... param_1_N1     ...      ...    text1 text2 ...

So MacroName2 and MacroName3 disappear, they have a delimiter role to the macro MacroName1, the text placed between the tags is found as an argument of MacroName1 and the possible parameters of MacroName1, MacroName2 and MacroName3 are grouped together.

This tree is then transformed into a call to the MacroName1 function whose parameters are strings:

    function Function1(pa, ar)
    -- 
    end

* pa is of type string[][]; it represents the list of groups of parameters for each tag, each parameter being a string.
* ar is of type string[]; it represents the list of arguments of the macro. Each argument is a text
* optionnaly po is of type string[]; corresponds to the list of tag types of the macro (useful when the macro consists of several intermediate tags)

All is said for the representation of the parameters.

Let's give a simple example:

   bla1 [color+|blue] the world is blue [color-|X] bla2

in the file macro.txt:

    -- <<<<<<
    -- ||||||2|color+|color-
    function outColor(pa, ar)
    local oList = {}
    local col="green"
    local text=''
    if type(pa) == "table" and #pa > 1 and type(pa[1]) == "table" and #pa[1]>0  and type(pa[2]) == "table" and #pa[2]>0 then
      local p1 = pa[1]
      local p2 = pa[2]
      if type(p1[1]) == "string" then
        col=p1[1]
      end
      if type(p2[1]) == "string" then
        text=p2[1]
      end
    end
    table.insert(oList, '\1<span style="color: ' .. col .. ';">\4')
    table.insert(oList, text)
     if type(ar) == "table" and #ar > 0 and type(ar[1]) == "string" then
       table.insert(oList, ar[1])
     end
    table.insert(oList, '\1</span>\4')
    return oList
    end
    -- >>>>>>

The result:

<span style="color: blue;">X the world is blue </span>

The representation in memory:

    color    
      |                 \                   \
    param block 1    param block 2        arguments
      |                   |                  |
    blue                  X            the world is blue 

The invisible characters \1 and \4 are used in HTML code generation to protect special characters <, >, etc., which will not be converted into HTML entities at the end of the process if they are placed between \1 and \4.


## Compilation

The compilation works with clang or gcc on Linux. You need to install the cpp-utf8 and lua-dev libraries from version 5.1 (on GitHub, it installs version 5.3).

## Use

Locate a macro file and in a command shell type:

    path_to_Bracketext\Bracketext.exe -m path_to_macros\macros.txt -f path_to_input_text_file\input.txt -o path_to_output_text_file\output.txt

The required options `-m` `-f` `-o` specify the location of the macro, text input, and output files.

It should be noted that all paths must be absolute.
