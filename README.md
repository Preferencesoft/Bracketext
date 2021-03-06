# Bracketext

## Text macros language

Bracketext is a macro language using only 4 symbols [ | ] and \' acting on the text in which the macros are placed when they are interpreted.

These symbols are used to form tags. These are always in the form:

[tag_name] or [tag_name|param1|...|param_n].

A macro can be defined by a single tag or by a sequence of several tags (as before) in the form:

[begin] or [begin] ... [end]

or more generally:

[begin]       [t1]      [t2]       [tn]       [end]

The ti tags can be repeated; the parts of text placed between each tag are considered the arguments of the macro. If the macro consists of only one tag, it will have no arguments.

These macros must be written in scripting languages like **JavaScript**. To write macros, you need to use *JavaScript* programming language. The project uses **JavaScriptEngineSwitcher**.

We use a scripting language to create macros that when inserted into any text will transform it into HTML for example.

![Bracketext logo](https://github.com/Preferencesoft/Bracketext/blob/master/b1.png)

Let's give an example. If we declare in the file ``macros.txt``:

    // <<<<<<
    // ||||||2|h1|/h1
    function outH1(...params) {
       var pa = getParameters(params);
       var a=pa[1];
       var t=pa[2];
       var oList = [];
       oList.push('-2');
       oList.push('<h1>');
       if (a.length>0) {if (a[0].length>0){
          a=a[0];
          var len=a.length;
          for (var i=0; i<len; i+=2) {
             var str=a[i+1];
             var tagNumber=a[i];
             if (tagNumber === '-1'){str=toHTML(str);}
                oList.push('-2');
             oList.push(str);
       }}}
       oList.push('-2');
       oList.push('</h1>');
       return stringListToXML(oList);
    }
    // >>>>>>

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

    // <<<<<<
    // ||||||2|color|/color
    function outColor(...params) {
    var p = getParameters(params);
    var a=p[1][0];
    var oList = [];
    var col="verdana";
    if (p[0][0].length > 0){if (p[0][0][0].length > 0){col=toString(p[0][0][0]);}}
    oList.push('-2');
    oList.push('<span style="color: '+col+';">');
    var len=a.length;
    for (var i=0; i<len; i+=2) {
    var str=a[i+1];
    if (a[i] === '-1'){str=toHTML(str);}
    oList.push('-2');
    oList.push(str);
    }
    oList.push('-2');
    oList.push('</span>');
    return stringListToXML(oList);
    }
    // >>>>>>

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
