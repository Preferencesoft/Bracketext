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
Create a macro file that closely follows the BBcode markup language, for those who use it or have used it before.

- Update the explanations in this README and create detailed documentation.

- I also plan to add the ability for macros to use multiple intermediate tags, as in the following example:
[command] text1 [line] tex2 [col] text3 [col] text4 [col] [line]  [/command]
The tags [line] and [col] must not, of course, be used as command tags or as intermediate tags for other commands. 
But the difficulty that arises is being able to find the type of intermediate tag (in the example [col] or [line]) in the parameters,
 which I didn't need until now.

Progress:

- I simplified the calling of Lua routines. Now the pa and ar parameters are respectively tables of tables of character strings and tables of character strings.

- The `macros.txt` macro command file (macros.txt) has finally been translated into the Lua language and needs to be checked further.

- You can now declare global variables in the macros.txt file by placing them in a field of the form: 
-- <<<<<< 
-- ||||||v| 
local counter=1 
local n=12334 
-- >>>>>> 

Bracketext is a macro language using only 4 symbols [ | ] and \ acting on the text in which the macros are placed when they are interpreted.
efl
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

For example \`[Example\`] will produce the text [example]

The general syntax of a macro is as follows... 

[MacroName1|param_1_1|param_1_2| ... |param_1_N1] Text1 [MacroName2|param_2_1|param_2_2| ... |param_2_N2] Text2 [MacroName3|param_3_1|param_3_2| ... |param_3_N3] ...

We suppose that MacroName1, MacroName2 and MacroName3 are linked in the macros.txt file:

    // <<<<<<
    // ||||||3|MacroName1|MacroName2|MacroName3

The number 3 indicates that MacroName2 can be repeated between MacroName1 and MacroName3.

Such a macro is represented by Bracketext as a tree and transformed into a call to a PowerShell function:

    MacroName1
        |                        \        \           \
      param1                    param2   param3     arguments
        |      |    \            /|\      /|\       /   |   \
    param_1_1 ... param_1_N      ...      ...    text1 text1 text3

So MacroName2 and MacroName3 disappear, they have a delimiter role to the macro MacroName1, the text placed between the tags is found as an argument of MacroName1 and the possible parameters of MacroName1, MacroName2 and MacroName3 are grouped together.

This tree is then transformed into a call to the MacroName1 function whose parameters are strings:

    function Function1(...params)
    
    var pa = getParameters(params);
    var p=p[0];
    var a=p[1];
    var t=p[2];

* p is of type string[][][]; it represents the list of groups of parameters for each tag, each parameter being represented by a list of text parts.
* a is of type string[][]; it represents the list of arguments of the macro. Each argument is a list of text parts
* t is of type string[]; corresponds to the list of tag types of the macro (useful when the macro consists of several intermediate tags)

The list of text parts is passed to the JavaScript functions in the form:

"-1", "characters_1", "-2", "characters_2", "-1", "characters_3", ..., "-2", "characters_n".

* "-1" means that the following characters have not yet been processed. 
* "-2" means the opposite to avoid applying the same conversion (to HTML for example) several times.

All is said for the representation of the parameters.

Let's give a simple example:

   bla1 [color|blue] the world is blue [color-|X] bla2

in the file macro.txt:

    -- <<<<<<
    -- ||||||2|color|/color
    function outColor(pa, ar)
    local oList = {}
    local col="verdana"
    if type(pa) == "table" and #pa > 0 and type(pa[1]) == "table" and #pa[1]>0 then
      local p = pa[1]
      if type(p[1]) == "string" then
        col=p[1]
      end
    end
    table.insert(oList, '\1<span style="color: ' .. col .. ';">\4')
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


## Compilation

This first version works. 

To compile the project, you need ``Visual Studio`` and install the ``JavaScriptEngineSwitcher`` and ``ChakraCore`` Nugets.

## Use

Locate a macro file and in a command shell type:

    path_to_Bracketext\Bracketext.exe -m path_to_macros\macros.txt -f path_to_input_text_file\input.txt -o path_to_output_text_file\output.txt

The required options `-m` `-f` `-o` specify the location of the macro, text input, and output files.

It should be noted that all paths must be absolute.

## To Do

* More macro files (``BBcode``)

* HTML Templates

* Calls to external JavaScript libraries 

* some converters to ``Markdown`` for example.
