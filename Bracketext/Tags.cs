using JavaScriptEngineSwitcher.Core;
using System;
using System.Collections.Generic;
using System.IO;
// using System.Management.Automation;
using System.Text;

// using JavaScriptEngineSwitcher.Core;
using JavaScriptEngineSwitcher.Core.Resources;
using JavaScriptEngineSwitcher.Core.Helpers;
using JavaScriptEngineSwitcher.ChakraCore;

namespace Bracketext
{
    class Tags
    {
        /*
         * tagNumber/tag
            None -1
            String -2
            OpenBracket -3
            ClosedBracket -4
            StraightLine -5

        */

        public const int nString = -1; // string in parameters
        public const int nResult = -2; // during a conversion, certain character strings should only be converted once
        public const int nOpenBracket = -3;
        public const int nClosedBracket = -4;
        public const int nStraightLine = -5;
        public const int nParameterBlocks = -7; // contains a complete tag
        public const int nArguments = -8; // contains a complete tag
        public const int nGroup = -9; // groups together successions of tags and strings (without [ | ])
        public const int nTag = -6; // tag
        public const int nMATag = -11; // tag argument to complete
        public const int nNone = -12;
        // we do not define a tag when the parameters are not complete

        public struct Entity
        {
            public int tagNumber;
            public string str; //nul when the entity is a tag
            // public List<Parameter> parameterList;
            public List<Entity> entityList;
        }

        // Tag association table
        // 0 undefined
        // 1 SINGLE (macro or function)
        // 2 BEGIN_END
        // 3 BEGIN_MIDDLE_END (deleted)
        // 4 BEGIN_REPEATED_MIDDLE_END
        // 5 BEGIN_REPEATED_AT_LEAST_ONCE_MIDDLE_END
        // (not yet implemented)

        const int TUndefined = 0;
        const int TSingle = 1;
        const int TBeginEnd = 2;
        const int TBeginMiddleEnd = 3;
        const int TBeginRepeatedMiddleEnd = 4;

        // Tag types in the same association.
        // "/" Beginning
        // "1", ..., "9" from 1 to 9 intermediate tag number
        // only "1" and "2" are currently being used
        // "." end

        const string TB = "/"; 
        const string TE = ".";

        /*
         * List that was originally used for testing.
         * 
        List<string[]> tagInfoList = new List<string[]>
        {
            new string[]{ "1", "Hello"},
            new string[]{ "2", "title", "/title"},
            new string[]{ "2", "h1", "/h1"},
            new string[]{ "2", "h2","/h2"},
            new string[]{ "2", "h3","/h3"},
            new string[]{ "2", "h4","/h4"},
            new string[]{ "2", "h5","/h5"},
            new string[]{ "2", "h6","/h6"},
            new string[]{ "2", "b","/b"},
            new string[]{ "2", "i","/i"},
            new string[]{ "2", "u","/u"},
            new string[]{ "2", "s","/s"},
            new string[]{ "2", "size","/size"},
            new string[]{ "2", "style","/style"},
            new string[]{ "2", "color","/color"},
            new string[]{ "2", "center", "/center"},
            new string[]{ "2", "left","/left"},
            new string[]{ "2", "right","/right"},
            new string[]{ "2", "quote","/quote"},
            new string[]{ "2", "url","/url"},
            new string[]{ "2", "img","/img"},
            new string[]{ "2", "ul","/ul"},
            new string[]{ "2", "ol","/ol"},
            new string[]{ "2", "li","/li"},
            new string[]{ "2", "code","/code"},
            new string[]{ "2", "table","/table"},
            new string[]{ "2", "tr","/tr"},
            new string[]{ "2", "th","/th"},
            new string[]{ "2", "td","/td"},
            new string[]{ "2", "youtube", "/youtube"},
            new string[]{ "3", "li","-", "/li"},
            new string[]{ "4", "list","*", "/list"},
        };
        */
        List<string[]> tagInfoList = new List<string[]>();
        // list of information about tags
        // list of information about tags
        List<string> commandList = new List<string>();
        // list of commands associated with tags
        List<string> functionNameList = new List<string>();
        // command name list

        readonly List<string> scriptList = new List<string>();
        // list of functions of global scope

        int[][] tagAssociationList;
        string[] tagList;
        int[] tagEntryList;
        int[] tagTypeList;
        int[] tagPositionList;
        List<Entity> document = new List<Entity>();

        public string SubStr(string s, int n)
        {
            return (s.Length >= n ? s.Substring(0, n) : "");
        }

        public void LoadMacros(string psFileName)
        {
            // provisionally, we load the entire text.
            string[] text = File.ReadAllText(psFileName, Encoding.Unicode).Split('\n');
            int state = 0;
            StringBuilder sb = new StringBuilder();
            // 0 no function
            // 1 header
            // 2 inside the function
            for (int i = 0; i < text.Length; i++)
            {
                if (state == 0)
                {
                    if (text[i].Trim() != "// <<<<<<")
                        continue;
                    state++;
                }
                else
                {
                    if (state == 1)
                    {
                        // beginning of a function
                        string header = text[i].Trim();
                        if (header.Substring(0, 9) != "// ||||||")
                            continue;
                        header = header.Substring(9);
                        var entry = header.Split('|');
                        bool isGlobal = false;
                        if (entry.Length > 0)
                        {
                            if (entry[0] == "g")
                                isGlobal = true;
                        }
                        if (isGlobal)
                        {
                            sb.Clear();
                            state = 4;
                            // pass 2 and 3
                        }
                        else
                        {
                            tagInfoList.Add(entry);
                            state++;
                            // goto 2
                        }
                    }
                    else
                    {
                        if (state == 2)
                        {
                            var fun = text[i].Trim();
                            if (SubStr(fun, 9) != "function ")
                                continue;
                            fun = fun.Substring(9);
                            int ind = fun.IndexOf("(");
                            fun = fun.Remove(ind);

                            functionNameList.Add(fun);
                            sb.Clear();
                            sb.Append(text[i].Trim());
                            state++;
                        }
                        else
                        {
                            if (state == 3)
                            {
                                if (text[i].Trim() != "// >>>>>>")
                                {
                                    sb.Append(text[i].Trim());
                                    continue;
                                }
                                else
                                {
                                    commandList.Add(sb.ToString());
                                    state = 0;
                                }
                            } else
                            {
                                if (state == 4)
                                {
                                    if (text[i].Trim() != "// >>>>>>")
                                    {
                                        sb.Append(text[i].Trim());
                                        continue;
                                    }
                                    else
                                    {
                                        scriptList.Add(sb.ToString());
                                        state = 0;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        public void Init()
        {
            List<System.Tuple<string, int, int>> tupleList = new List<System.Tuple<string, int, int>>();
            int sepListCount = tagInfoList.Count;
            tagAssociationList = new int[sepListCount][];
            tagTypeList = new int[sepListCount];
            for (int n = 0; n < sepListCount; n++)
            {
                string[] sep = tagInfoList[n];
                int len = sep.Length;
                for (int i = 1; i < len; i++)
                {
                    tupleList.Add(System.Tuple.Create<string, int, int>(sep[i], n, i));
                }
            }
            // sort by string
            tupleList.Sort((a, b) => (a.Item1.CompareTo(b.Item1)));
            int tupleListCount = tupleList.Count;
            tagList = new string[tupleListCount];
            tagEntryList = new int[tupleListCount];
            tagPositionList = new int[tupleListCount];

            for (int i = 0; i < tupleListCount; i++)
            {
                var t = tupleList[0];
                tagList[i] = t.Item1;
                tagEntryList[i] = t.Item2;
                tagPositionList[i] = t.Item3;
                tupleList.RemoveAt(0);
            }

            for (int n = 0; n < sepListCount; n++)
            {
                string[] sep = tagInfoList[n];
                int len = sep.Length;
                tagAssociationList[n] = new int[len - 1];
                for (int i = 1; i < len; i++)
                {
                    int index = Array.BinarySearch(tagList, sep[i]);
                    tagAssociationList[n][i - 1] = index;
                }
                bool isInt = int.TryParse(sep[0], out int info);
                tagTypeList[n] = info;
            }
        }

      

        /*
        readonly string script = @"function Test-Me($param1) {$w=[string[][]]$param1;$x=$w[0][2];""Hello from Test-Me with , $x"" }";
        public void PS()
        {
            using (var powershell = PowerShell.Create())
            {
                powershell.AddScript(script, false);

                powershell.Invoke();

                powershell.Commands.Clear();

                // powershell.AddCommand("Test-Me").AddParameter("param1", new[] { new[] { "12", "a", "zer" }, new[] { "12", "a", "zer" }, new[] { "12", "a", "zer" } } );

                var results = powershell.Invoke();
                Console.WriteLine(results[0]);
            }
        }
        */

        /*
         // [? 1
         // |? 2
         // ]? 3
         // [[ -1
         // || -2
         // ]] -3
         // other 0
         */

        public bool IsNotDelim(char c)
        {
            switch (c)
            {
                case '[':
                case ']':
                case '|':
                    return false;
            }
            return true;
        }

        public bool IsNotSymbol(char c)
        {
            switch (c)
            {
                case '[':
                case ']':
                case '|':
                case '`':
                    return false;
            }
            return true;
        }

        public int CharToTagNumber(char c)
        {
            switch (c)
            {
                case '[':
                    return nOpenBracket;
                case ']':
                    return nClosedBracket;
                case '|':
                    return nStraightLine;
            }
            return nNone;
        }

        public char TagNumberToChar(int t)
        {
            switch (t)
            {
                case nOpenBracket:
                    return '[';
                case nClosedBracket:
                    return ']';
                case nStraightLine:
                    return '|';
                case nString:
                    return 's';
                case nTag:
                    return 't';
                case nGroup:
                    return 'g';
                case nParameterBlocks:
                    return 'p';
                case nArguments:
                    return 'a';
            }
            return ' ';
        }

        // Delete CR characters
        // but actually does nothing.
        public string CleanTag(string t)
        {
            return t;
        }

        public void BBCodeToTree()
        {
            // search for the tags without parameters [command]
            for (int i = 0; i < document.Count - 2; i++)
            {
                if (document[i].tagNumber == nOpenBracket
                    && document[i + 1].tagNumber == nString
                    && document[i + 2].tagNumber == nClosedBracket
                    )
                {
                    // searching for the tag
                    string t = CleanTag(document[i + 1].str);
                    int tNumber = Array.BinarySearch(tagList, t);
                    if (tNumber >= 0)
                    {
                        document[i] = new Entity()
                        {
                            tagNumber = nMATag,
                            str = null,
                            entityList = new List<Entity>()
                            {
                                new Entity()
                                {
                                    tagNumber = tNumber,
                                    str = null,
                                    entityList = new List<Entity>()
                                    {
                                        new Entity()
                                        {
                                            tagNumber = nParameterBlocks,
                                            str = TE,
                                            entityList = null
                                        }
                                    }
                                }
                            }
                        };
                        document.RemoveAt(i + 1);
                        document.RemoveAt(i + 1);
                    }
                }
            }
            // DisplayEntity(document);
            // reduction
            bool notEnded = true;
            while (notEnded)
            {
                notEnded = false;
                // grouping inside parameters |...| or |...] or ]...]
                // We group together packets made up exclusively of nTag, nString and nGroup
                bool modified = true;
                while (modified)
                {
                    int i = 0;
                    modified = false;
                    while (i < document.Count - 1)
                    {
                        switch (document[i].tagNumber)
                        {
                            case nClosedBracket:
                                bool found = false;
                                int j;
                                for (j = i + 1; j < document.Count; j++)
                                {
                                    int tn = document[j].tagNumber;
                                    //if (tn == nClosedBracket || tn == nOpenBracket)
                                    if (tn == nClosedBracket)
                                    {
                                        found = true;
                                        break;
                                    }
                                    else
                                        if (tn != nTag && tn != nString && tn != nGroup)
                                        break;
                                }
                                if (found)
                                {
                                    ReductionG(ref i, j, out modified);
                                    notEnded = modified;
                                }
                                break;
                            case nStraightLine:
                                found = false;
                                for (j = i + 1; j < document.Count; j++)
                                {
                                    int tn = document[j].tagNumber;
                                    if (tn == nClosedBracket || tn == nStraightLine)
                                    {
                                        found = true;
                                        break;
                                    }
                                    else
                                        if (tn != nTag && tn != nString && tn != nGroup)
                                        break;
                                }
                                if (found)
                                {
                                    ReductionG(ref i, j, out modified);
                                    notEnded = modified;
                                }
                                break;
                        }
                        i++;
                    }
                }
                // DisplayEntity(document);

                // scan [string|group|...|group]
                int ii = 0;
                int tNum = 0;
                while (ii < document.Count - 5)
                {
                    if (document[ii].tagNumber == nOpenBracket
                        && document[ii + 1].tagNumber == nString
                        && document[ii + 2].tagNumber == nStraightLine)
                    {
                        // searching for the tag
                        string t = CleanTag(document[ii + 1].str);
                        tNum = Array.BinarySearch(tagList, t);
                        bool foundInstruction = false;
                        int j = 0;
                        if (tNum >= 0)
                        {
                            for (j = ii + 3; j < document.Count - 1; j += 2)
                            {
                                if (document[j].tagNumber == nGroup)
                                {
                                    if (document[j + 1].tagNumber == nStraightLine)
                                        continue;
                                    else
                                        if (document[j + 1].tagNumber == nClosedBracket)
                                    {
                                        j += 2;
                                        foundInstruction = true;
                                    }
                                }
                                break;
                            }
                        }
                        if (foundInstruction)
                        {
                            notEnded = true;
                            Entity e = new Entity()
                            {
                                tagNumber = nMATag,
                                str = null,
                                entityList = new List<Entity>()
                                {
                                    new Entity()
                                    {
                                        tagNumber = tNum,
                                        str = null,
                                        entityList = new List<Entity>()
                                        {
                                            new Entity()
                                            {
                                                tagNumber = nParameterBlocks,
                                                str = TE,
                                                // By default, the first parameter block is defined as terminal TE
                                                entityList = new List<Entity>()
                                            }
                                        }
                                    }
                                }
                            };
                            for (int k = ii + 3; k < j; k += 2)
                                e.entityList[0].entityList[0].entityList.Add(document[k]);
                            for (int k = ii + 1; k < j; k++)
                                document.RemoveAt(ii + 1);
                            document[ii] = e;
                        }
                    }
                    ii++;
                }
                // DisplayEntity(document);

                // Search for complete associated tags [begin] ... [end] and others
                modified = true;
                while (modified)
                {
                    modified = false;
                    ii = 0;
                    while (ii < document.Count)
                    {
                        if (document[ii].tagNumber == nMATag)
                        {
                            Entity tag = document[ii].entityList[0];
                            int position = tagPositionList[tag.tagNumber];
                            //string s = tagList[tag.tagNumber];
                            if (position == 1)
                            {
                                // When the position equals 1, we have a starting tag [begin]
                                int entry = tagEntryList[tag.tagNumber];
                                int type = tagTypeList[entry];
                                int[] asso = tagAssociationList[entry];
                                int last = asso.Length - 1;
                                bool f = false;
                                switch (type)
                                {
                                    // See the tag association table, above.
                                    case TSingle:
                                        {
                                            modified = true;
                                            notEnded = true;
                                            Entity newTag = new Entity()
                                            {
                                                tagNumber = nTag,
                                                str = null,
                                                entityList = document[ii].entityList
                                                // We're retrieving the parameter block from the current tag.
                                            };
                                            newTag.entityList[0].entityList.Add(
                                                new Entity()
                                                {
                                                    tagNumber = nArguments,
                                                    str = null,
                                                    entityList = new List<Entity>()
                                                });
                                            // No argument here
                                            document[ii] = newTag;
                                        }
                                        break;
                                    case TBeginEnd:
                                        {
                                            int j;
                                            for (j = ii + 1; j < document.Count; j++)
                                            {
                                                int tn = document[j].tagNumber;
                                                if (tn == nMATag)
                                                {
                                                    if (document[j].entityList[0].tagNumber == asso[last])
                                                    {
                                                        f = true;
                                                        break;
                                                    }
                                                    else
                                                        break;
                                                }
                                                else
                                                    if (tn != nString && tn != nTag)
                                                    break;
                                            }
                                            if (f)
                                            {
                                                modified = true;
                                                notEnded = true;
                                                Reduction1(ii, j);
                                            }
                                        }
                                        break;
                                    case TBeginMiddleEnd:
                                    case TBeginRepeatedMiddleEnd:
                                        {
                                            int count = 0;
                                            int j;
                                            for (j = ii + 1; j < document.Count; j++)
                                            {
                                                int tn = document[j].tagNumber;
                                                if (tn == nMATag)
                                                {
                                                    if (document[j].entityList[0].tagNumber == asso[last])
                                                    {
                                                        f = true;
                                                        break;
                                                    }
                                                    else
                                                        if (document[j].entityList[0].tagNumber != asso[last - 1])
                                                        break;
                                                    else count++;
                                                }
                                                else
                                                    if (tn != nString && tn != nTag)
                                                    break;
                                            }
                                            if (type == TBeginRepeatedMiddleEnd && count == 0)
                                                f = false;
                                            if (f)
                                            {
                                                modified = true;
                                                notEnded = true;
                                                if (count == 0)
                                                    Reduction1(ii, j);
                                                else
                                                    Reduction2(ii, j, last, asso);
                                            }
                                        }
                                        break;
                                }
                            }
                        }
                        ii++;
                    }
                }
            }

            // *** Test ***

            // DisplayEntity(document);
        }

        // Reduced to form a group.
        public void ReductionG(ref int i, int j, out bool modified)
        {
            modified = false;
            // case already one group
            if (j - i == 2)
                if (document[i + 1].tagNumber == nGroup)
                    return;
            modified = true;
            if (j - i >= 2)
            {
                Entity gr = new Entity()
                {
                    tagNumber = nGroup,
                    str = null,
                    entityList = new List<Entity>()
                };
                for (int k = i + 1; k < j; k++)
                {
                    if (document[k].tagNumber == nGroup)
                        gr.entityList.AddRange(document[k].entityList);
                    else
                        gr.entityList.Add(document[k]);
                }
                for (int k = i + 2; k < j; k++)
                    document.RemoveAt(i + 1);
                document[i + 1] = gr;
                ++i;
            }
            else
            {
                document.Insert(i + 1,
                    new Entity()
                    {
                        tagNumber = nGroup,
                        str = null,
                        entityList = new List<Entity>()
                        {
                            new Entity()
                            {
                                tagNumber = nString,
                                str = "",
                                entityList = new List<Entity>()
                            }
                        }
                    }
                    );
                ++i;
            }
        }

        public void Reduction1(int ii, int j)
        {
            // The argument is removed
            Entity argumentGroup = new Entity()
            {
                tagNumber = nGroup,
                str = null,
                entityList = new List<Entity>()
            };
            for (int k = ii + 1; k < j; k++)
                argumentGroup.entityList.Add(document[k]);
            for (int k = ii + 1; k < j; k++)
                document.RemoveAt(ii + 1);
            // The parameters of the two tags
            Entity newTag = new Entity()
            {
                tagNumber = nTag,
                str = null,
                entityList = new List<Entity>()
                                            {
                                               new Entity
                                               {
                                                   tagNumber = document[ii].entityList[0].tagNumber,
                                                   str=null,
                                                   entityList = new List<Entity>()
                                               }
                                            }
            };
            List<Entity> param = document[ii].entityList[0].entityList;
            if (param != null)
            {
                // newTag.entityList[0].entityList.AddRange(param);
                newTag.entityList[0].entityList.Add(new Entity
                {
                    tagNumber = nParameterBlocks,
                    str = TB,
                    entityList = param[0].entityList
                });
                // The first block is of the beginning type.
            }
            param = document[ii + 1].entityList[0].entityList;
            if (param != null)
                // newTag.entityList[0].entityList.AddRange(param);
                newTag.entityList[0].entityList.Add(new Entity
                {
                    tagNumber = nParameterBlocks,
                    str = TE,
                    entityList = param[0].entityList
                });
            newTag.entityList[0].entityList.Add(
                new Entity
                {
                    tagNumber = nArguments,
                    str = null,
                    entityList = new List<Entity>() { argumentGroup }
                }
                );
            document[ii] = newTag;
            document.RemoveAt(ii + 1);
        }

        public void Reduction2(int ii, int j, int last, int[] asso)
        {
            Entity newTag = new Entity()
            {
                tagNumber = nTag,
                str = null,
                entityList = new List<Entity>()
                                            {
                                               new Entity
                                               {
                                                   tagNumber = document[ii].entityList[0].tagNumber,
                                                   str=null,
                                                   entityList = new List<Entity>()
                                               }
                                            }
            };


            List<Entity> argumentList = new List<Entity>();
            Entity subGroup = new Entity()
            {
                tagNumber = nGroup,
                str = null,
                entityList = new List<Entity>()
            };
            if (document[ii].entityList[0].entityList != null)
            {
                newTag.entityList[0].entityList.Add(new Entity
                {
                    tagNumber = nParameterBlocks,
                    str = TB,
                    entityList = document[ii].entityList[0].entityList[0].entityList
                });
                // The first block is of the beginning type.
            }
            for (int k = ii + 1; k <= j; k++)
            {
                int tn = document[k].tagNumber;
                if (tn == nMATag)
                {
                    var tg = document[k].entityList[0].tagNumber;
                    bool isIntermediateTag = false;
                    string tagType = TE;
                    if (tg == asso[last - 1])
                    {
                        tagType = "1";
                        isIntermediateTag = true;
                    }
                    else
                    {
                        if (tg == asso[last])
                            isIntermediateTag = true;
                    }
                    if (isIntermediateTag)
                    {
                        if (document[k].entityList[0].entityList != null)
                        {
                            // newTag.entityList[0].entityList.AddRange(document[k].entityList[0].entityList);
                            newTag.entityList[0].entityList.Add(new Entity
                            {
                                tagNumber = nParameterBlocks,
                                str = tagType,
                                entityList = document[k].entityList[0].entityList[0].entityList
                            });
                        }
                        argumentList.Add(subGroup);
                        subGroup = new Entity()
                        {
                            tagNumber = nGroup,
                            str = null,
                            entityList = new List<Entity>()
                        };
                        continue;
                    }
                }
                subGroup.entityList.Add(document[k]);
            }
            for (int k = ii + 1; k <= j; k++)
                document.RemoveAt(ii + 1);
            newTag.entityList[0].entityList.Add(
                new Entity
                {
                    tagNumber = nArguments,
                    str = null,
                    entityList = argumentList
                });
            document[ii] = newTag;
            //document.RemoveAt(ii + 1);
        }



        private string TagToString(Entity e)
        {
            switch (e.tagNumber)
            {
                case nStraightLine:
                    return "|";
                case nOpenBracket:
                    return "[";
                case nClosedBracket:
                    return "]";
                case nString:
                    return e.str;
                case nMATag:
                    return "[not recognized tag]";
            }
            return string.Format("[error] {0}", e.tagNumber);
        }

        private List<string> ToStringArrayList(List<Entity> le, int[] index)
        {
            List<string> res = new List<string>();
            int i = -1;
            string str = "";
            if (index.Length>0) str = index[0].ToString();
            for (int j = 1; j < index.Length; j++)
                str += "," + index[j].ToString();
            foreach (Entity e in le)
            {
                if (e.tagNumber==nResult)
                {
                    i++;
                    res.Add(str+ "," + i.ToString());
                    res.Add(nResult.ToString());
                    i++;
                    res.Add(str+ "," + i.ToString());
                    res.Add(e.str);
                }
                else
                {
                    i++;
                    res.Add(str + "," + i.ToString());
                    res.Add(nString.ToString());
                    i++;
                    res.Add(str + "," + i.ToString());
                    res.Add(TagToString(e));       
                }
            }
            return res;
        }

        public string[] GetParameters(Entity tag)
        {
            List<string> args = new List<string>{ };
            //List<string[][]> args = new List<string[][]>();
            List<string> pars = new List<string>();
            List<string> par = new List<string>();
            if (tag.tagNumber == nTag)
            {
                List<Entity> pList = tag.entityList[0].entityList;
                int[] indexP = new int[3] { 0, -1, -1 };
                // parameter indexes
                int[] indexA = new int[2] { 1, -1 };
                // argument indexes
                int indexT = -1;
                // type index
                foreach (Entity pa in pList)
                {
                    if (pa.tagNumber == nParameterBlocks)
                    {
                        indexT++;
                        pars.Add(string.Format("2,{0}", indexT));
                        pars.Add(string.Format("{0}", pa.str));
                        if (pa.entityList != null)
                        {
                            indexP[1]++;
                            // List<string> ls = new List<string>();
                            par.Clear();
                            foreach (Entity grp in pa.entityList)
                            {
                                if (grp.entityList != null)
                                {
                                    indexP[2]++;
                                    par.AddRange(ToStringArrayList(grp.entityList, indexP));
                                }
                            }
                            pars.AddRange(par.ToArray());
                        }
                    }
                    if (pa.tagNumber == nArguments)
                    {
                        if (pa.entityList != null)
                        {
                            foreach (Entity gra in pa.entityList)
                            {
                                if (gra.entityList == null) continue;
                                indexA[1]++;
                                pars.AddRange(ToStringArrayList(gra.entityList, indexA));
                            }
                        }
                    }
                }
            }
            return pars.ToArray();
        }


        public string DocumentToHTML()
        {
            StringBuilder sb = new StringBuilder();
            foreach (Entity e in document)
            {
                sb.Append(e.str);
            }
            return sb.ToString();
        }

        private string HTMLEntities(string s)
        {
            int n = s.Length;
            string str = "";
            for (int i = 0; i < n; i++)
            {
                var res = s[i].ToString();
                switch (s[i])
                {
                    case ' ':
                        res = " ";
                        break;
                    case '<':
                        res = "&lt;";
                        break;
                    case '>':
                        res = "&gt;";
                        break;
                    case '&':
                        res = "&amp;";
                        break;
                    case '"':
                        res = "&quot;";
                        break;
                    case '\'':
                        res = "&apos;";
                        break;
                    case '¢':
                        res = "&cent;";
                        break;
                    case '£':
                        res = "&pound;";
                        break;
                    case '¥':
                        res = "&yen;";
                        break;
                    case '€':
                        res = "&euro;";
                        break;
                    case '©':
                        res = "&copy;";
                        break;
                    case '®':
                        res = "&reg;";
                        break;
                }
                str += res;
            }
            return str;
        }

            // Replace symbol tag ([, |, ]) by a <string>
            void SymbolTagToString(List<Entity> eList, int pos)
        {
            switch (eList[pos].tagNumber)
            {
                case nStraightLine:
                case nOpenBracket:
                case nClosedBracket:
                    eList[pos] = new Entity()
                    {
                        tagNumber = nString,
                        str = TagNumberToChar(eList[pos].tagNumber).ToString(),
                        entityList = null
                    };
                    break;
            }
        }

        void SymbolTagToHTML(List<Entity> eList, int pos)
        {
            switch (eList[pos].tagNumber)
            {
                case nStraightLine:
                case nOpenBracket:
                case nClosedBracket:
                    eList[pos] = new Entity()
                    {
                        tagNumber = nString,
                        str = TagNumberToChar(eList[pos].tagNumber).ToString(),
                        entityList = null
                    };
                    break;
                case nString:
                case nResult:
                    eList[pos] = new Entity()
                    {
                        tagNumber = nString,
                        str = HTMLEntities(eList[pos].str),
                        entityList = null
                    };
                    break;
            }
        }

        private string StringifyMATag(Entity e)
        {
            // we loop into parameters
            StringBuilder sb = new StringBuilder("[");
            if (e.tagNumber <= 0)
            {
                sb.Append(tagList[e.entityList[0].tagNumber]);
                List<Entity> pList = e.entityList[0].entityList;
                // parameter list
                for (int j = 0; j < pList.Count; j++)
                {
                    Entity pa = pList[j];
                    if (pa.tagNumber == nParameterBlocks)
                    {
                        if (pa.entityList != null)
                        {
                            foreach (Entity pb in pList[j].entityList)
                            {
                                if (pb.entityList != null)
                                {
                                    foreach (Entity grp in pb.entityList)
                                    {
                                        if (grp.entityList != null)
                                        {
                                            for (int l = 0; l < grp.entityList.Count; l++)
                                            {
                                                // add elements
                                                sb.Append("|");
                                                sb.Append(grp.entityList[l]);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    /*
                    if (pa.tagNumber == nArguments)
                    {
                        if (pa.entityList != null)
                        {
                            foreach (Entity gra in pa.entityList)
                            {
                                if (gra.entityList != null)
                                {
                                    for (int l = 0; l < gra.entityList.Count; l++)
                                    {

                                    }
                                }
                            }
                        }
                    }
                    */
                }
                sb.Append("]");
            }
            return sb.ToString();
        }

        private struct Position
        {
            public List<Entity> list;
            public int index;
        };




        public void EvalTree()
        {
            IJsEngineSwitcher engineSwitcher = JsEngineSwitcher.Current;

            List<List<Position>> tagListList = new List<List<Position>>();
            int level = 0;
            tagListList.Add(new List<Position>());
            for (int i = 0; i < document.Count; i++)
            {
                var doc = document;
                if (document[i].tagNumber == nTag
                    || document[i].tagNumber == nMATag)
                    tagListList[level].Add(new Position { list = document, index = i });
                else
                    SymbolTagToString(doc, i);
            }
            // descent to the innermost tags
            while (true)
            {
                if (tagListList[level].Count == 0)
                    break;
                level++;
                tagListList.Add(new List<Position>());
                foreach (Position pos in tagListList[level - 1])
                {
                    Entity e = pos.list[pos.index];
                    if (e.tagNumber <= 0)
                    {
                        List<Entity> pList = e.entityList[0].entityList;
                        for (int j = 0; j < pList.Count; j++)
                        {
                            Entity pa = pList[j];
                            if (pa.tagNumber == nParameterBlocks)
                            {
                                if (pa.entityList != null)
                                {
                                    foreach (Entity pb in pList[j].entityList)
                                    {
                                        if (pb.entityList != null)
                                        {
                                            foreach (Entity grp in pb.entityList)
                                            {
                                                if (grp.entityList != null)
                                                {
                                                    for (int l = 0; l < grp.entityList.Count; l++)
                                                    {
                                                        if (grp.entityList[l].tagNumber == nTag
                                                            || grp.entityList[l].tagNumber == nMATag)
                                                            tagListList[level].Add(
                                                                new Position()
                                                                {
                                                                    list = grp.entityList,
                                                                    index = l
                                                                });
                                                        else
                                                            SymbolTagToString(grp.entityList, l);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            if (pa.tagNumber == nArguments)
                            {
                                if (pa.entityList != null)
                                {
                                    foreach (Entity gra in pa.entityList)
                                    {
                                        if (gra.entityList != null)
                                        {
                                            for (int l = 0; l < gra.entityList.Count; l++)
                                            {
                                                if (gra.entityList[l].tagNumber == nTag
                                                    || gra.entityList[l].tagNumber == nMATag)
                                                    tagListList[level].Add(
                                                        new Position()
                                                        {
                                                            list = gra.entityList,
                                                            index = l
                                                        });
                                                else
                                                    SymbolTagToString(gra.entityList, l);
                                                //SymbolTagToHTML(gra.entityList, l);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            tagListList.Reverse();
            List<string> results = new List<string>();
            string bigScript = "";
            foreach (string script in scriptList)
            {
                bigScript += script + "\n";
            }
            foreach (string script in commandList)
            {
                bigScript += script + "\n";
            }

            engineSwitcher.EngineFactories.AddChakraCore();
            engineSwitcher = JsEngineSwitcher.Current;
            engineSwitcher.DefaultEngineName = ChakraCoreJsEngine.EngineName;
            // engineSwitcher.EngineFactories.Add(new ChakraCoreJsEngineFactory());
            // IPrecompiledScript precompiledCode = null;
            using (var engine = engineSwitcher.CreateDefaultEngine())
            {
                if (!engine.SupportsScriptPrecompilation)
                {
                    Console.WriteLine("{0} версии {1} не поддерживает " +
                        "предварительную компиляцию скриптов!",
                        engine.Name, engine.Version);
                    return;
                }
                try
                {
                    // precompiledCode = engine.Precompile(bigScript);
                    engine.Execute(bigScript);
                    foreach (List<Position> pList in tagListList)
                    {
                        pList.Reverse();
                        foreach (Position p in pList)
                        {
                            var tagNum = p.list[p.index].entityList[0];
                            int entry = tagEntryList[tagNum.tagNumber];
                            var tag = p.list[p.index];
                            if (tag.tagNumber == nTag)
                            {
                                string[] args = GetParameters(tag);
                                var d = new Data();
                                var res0=engine.CallFunction(functionNameList[entry], args);
                                var r = d.Deserialize((string)res0);
                                // post execution?
                                // deals with load instruction 
                                if (r.Count > 1)
                                {
                                    if (r[0] == nNone.ToString() 
                                        && string.IsNullOrEmpty(r[1]))
                                    {
                                        int c = r.Count;
                                        for (int ind=2; ind<c; ind +=2)
                                        {
                                            if (ind + 3 < c)
                                            {
                                                if (r[ind] == nNone.ToString()
                                            && r[ind + 1] == "load")
                                                {
                                                    string text = System.IO.File.ReadAllText(r[ind + 3]);
                                                    r[ind] = nResult.ToString();
                                                    r[ind + 1] = text;
                                                    r.RemoveAt(ind + 2);
                                                    r.RemoveAt(ind + 2);
                                                    c -= 2;
                                                }
                                            }
                                        }
                                        r.RemoveAt(0);
                                        r.RemoveAt(0);
                                    }
                                }
                                List<Entity> res1 = new List<Entity>();
                                for (int i = 0; i < r.Count; i+=2)
                                {
                                    res1.Add(
                                        new Entity()
                                        {
                                            tagNumber = int.Parse(r[i]),
                                            str = r[i + 1]
                                        });
                                }
                                var res = res1.ToArray();
                                    /*
                                        powershell.AddCommand(functionNameList[entry])
                                            .AddParameter("param", GetParameters(tag).param)
                                            .AddParameter("arg", GetParameters(tag).arg);
                                      */
                                    /*
                                       powershell.AddCommand(functionNameList[entry])
                                             .AddParameter("param", tag.entityList[0].entityList)
                                             .AddParameter("arg", GetParameters(tag).arg);
                                             */
                                    // var res = powershell.Invoke();
                                    List<Entity> le = new List<Entity>();
                                for (int i = 0; i < res.Length; i++)
                                {
                                    // if (res[i] != null)
                                    {
                                        try
                                        {
                                            le.Add(new Entity()
                                            {
                                                tagNumber = res[i].tagNumber,
                                                str = res[i].str.ToString(),
                                                entityList = null
                                            });
                                        }
                                        catch
                                        {
                                            Console.WriteLine("error in " + functionNameList[entry]);
                                        }
                                    }
                                }
                                p.list.RemoveAt(p.index);
                                p.list.InsertRange(p.index, le);
                                /*
                                    for (int i=0; i<res.Count; i++)
                                    {
                                        if (res[i] != null)
                                        {
                                            try
                                            {
                                                le.Add(new Entity()
                                                {
                                                    tagNumber = (int)res[i].Members["tagNumber"].Value,
                                                    str = res[i].Members["str"].Value.ToString(),
                                                    entityList = null
                                                });
                                            } catch
                                            {
                                                Console.WriteLine("error in "+ functionNameList[entry]);
                                            }
                                        }
                                    }
                                    p.list.RemoveAt(p.index);
                                    p.list.InsertRange(p.index, le);
                                    */
                            }
                            else
                            {
                                // nMATag
                                p.list[p.index] = new Entity()
                                {
                                    tagNumber = nString,
                                    str = StringifyMATag(tag),
                                    entityList = null
                                };
                            }

                            // DisplayEntity(document);
                        }

                    }
                }
                catch (JsException e)
                {
                    Console.WriteLine("Во время работы JavaScript-движка произошла " +
                        "ошибка!");
                    Console.WriteLine();
                    Console.WriteLine(JsErrorHelpers.GenerateErrorDetails(e));
                    return;
                }

                // DisplayEntity(document);
            }
        }
        public void DisplayEntity(List<Entity> document)
        {
            if (document != null)
            {
                foreach (Entity e in document)
                {

                    if (e.tagNumber < 0)
                    {
                        switch (e.tagNumber)
                        {
                            case nNone:
                                Console.WriteLine(">>>>>None");
                                break;
                            case nString:
                                Console.WriteLine(">>>>>{0}", e.str);
                                break;
                            case nOpenBracket:
                                Console.WriteLine(">>>>>{0}", e.str);
                                break;
                            case nClosedBracket:
                                Console.WriteLine(">>>>>{0}", e.str);
                                break;
                            case nStraightLine:
                                Console.WriteLine(">>>>>{0}", e.str);
                                break;
                            case nTag:
                                Console.WriteLine(">>>>>BEGIN TAG");
                                DisplayEntity(e.entityList);
                                Console.WriteLine(">>>>>END TAG");
                                break;
                            case nMATag:
                                Console.WriteLine(">>>>>BEGIN MATAG");
                                DisplayEntity(e.entityList);
                                Console.WriteLine(">>>>>END MATAG");
                                break;
                            case nParameterBlocks:
                                Console.WriteLine(">>>>>BEGIN PARAMETER BLOCK");
                                DisplayEntity(e.entityList);
                                Console.WriteLine(">>>>>END PARAMETER BLOCK");
                                break;
                            case nArguments:
                                Console.WriteLine(">>>>>BEGIN ARGUMENTS");
                                DisplayEntity(e.entityList);
                                Console.WriteLine(">>>>>END ARGUMENTS");
                                break;
                            case nGroup:
                                Console.WriteLine(">>>>>BEGIN GROUP");
                                DisplayEntity(e.entityList);
                                Console.WriteLine(">>>>>END GROUP");
                                break;
                        }

                    }
                    else
                    {
                        Console.WriteLine("tag:{0}", tagList[e.tagNumber]);
                        if (e.entityList != null)
                        {
                            Console.WriteLine(">>>>>>>>>>");
                            DisplayEntity(e.entityList);
                            Console.WriteLine(">>>>>>>>>>");
                        }
                    }
                }
            }
        }
        public void ScanFile(string fileName)
        {
            string text = File.ReadAllText(fileName, Encoding.Unicode);
            Console.WriteLine(text);
            int textLength = text.Length;
            StringBuilder sb = new StringBuilder();
            int i = 0;
            while (i < textLength)
            {
                Char c = text[i];
                if (IsNotDelim(c))
                {
                    // retrieve the string
                    sb.Clear();
                    while (i < textLength)
                    {
                        c = text[i];
                        if (IsNotSymbol(c))
                            sb.Append(c);
                        else
                        {
                            if (c == '`')
                            {
                                if (++i < textLength)
                                {
                                    c = text[i];
                                    if (IsNotSymbol(c))
                                    {
                                        sb.Append('`');
                                        sb.Append(c);
                                    }
                                    else
                                    {
                                        // no ` before [ ] | `
                                        sb.Append(c);
                                    }
                                }
                                else
                                {
                                    sb.Append('`');
                                }
                            }
                            else
                                break;
                        }
                        ++i;
                    }
                    string s = sb.ToString();
                    document.Add(
                        new Entity()
                        {
                            tagNumber = nString,
                            str = s,
                            entityList = null
                        });
                    --i;
                    // Console.WriteLine(">> {0}", sb.ToString());
                }
                else
                {
                    // [ or ] or | is a delim
                    document.Add(
                                    new Entity()
                                    {
                                        tagNumber = CharToTagNumber(c),
                                        str = c.ToString(),
                                        entityList = null
                                    });
                    //Console.WriteLine("[ or ] or |");
                }
                ++i;
            }
            // *** Test ***
            /*
            foreach (Entity e in document)
            {
                if (e.tagNumber<0)
                Console.WriteLine("type:{0}",  CharToTagNumber(e.tagNumber));
                else
                    Console.WriteLine("type:{0}", tagList[e.tagNumber]);
                Console.WriteLine(">>>>>{0}", e.str);
            }
            */
        }
    }
}
