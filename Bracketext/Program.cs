﻿using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;

namespace Bracketext
{
    class Program
    {
        static void Main(string[] args)
        {
            string inputFile = "", outputFile = "", macroFile = "";
            int n = args.Length;
            bool error = false;
            Dictionary<string, string> dic = new Dictionary<string, string>();
            if (n > 1)
            {
                for (int i = 0; i < args.Length; i++)
                {
                    switch (args[i])
                    {
                        case "-f":
                            if (i + 1 < args.Length)
                                if (!dic.ContainsKey("input"))
                                {
                                    dic["input"] = args[i + 1];
                                }
                                else error = true;
                            else error = true;
                            break;
                        case "-o":
                            if (i + 1 < args.Length)
                                if (!dic.ContainsKey("output"))
                                {
                                    dic["output"] = args[i + 1];
                                }
                                else error = true;
                            else error = true;
                            break;
                        case "-m":
                            if (i + 1 < args.Length)
                                if (!dic.ContainsKey("macros"))
                                {
                                    dic["macros"] = args[i + 1];
                                }
                                else error = true;
                            else error = true;
                            break;
                    }
                    if (error) break;
                }
                if (dic.Keys.Count != 3) error = true;
            }
            else error = true;
            if (error)
            {
                Console.WriteLine("usage: Bracketext.exe -m macro_file -f input_file_path -o output_file_path");
                return;
            }
            inputFile = dic["input"];
            outputFile = dic["output"];
            macroFile = dic["macros"];
  
            var bb = new Tags();
            bb.LoadMacros(macroFile);
            bb.Init();
            bb.ScanFile(inputFile);
            bb.BBCodeToTree();
            bb.EvalTree();
            TextWriter txt = new StreamWriter(outputFile);
            txt.Write(bb.DocumentToHTML());
            txt.Close();
        }
    }
}
