using System;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;

namespace Bracketext
{
    class Program
    {
        static void Main(string[] args)
        {
            /*
            string[] parts = args;
            string inputFile, outputFile;
            if (args.Length > 1)
            {
                inputFile = args[0];
                outputFile = args[1];
            }
            else
            {
                Console.WriteLine("usage: Bracketext.exe input_file_path output_file_path");
                return;
            }
            */
            var bb = new Tags();
            bb.LoadMacros(@"C:\Users\prefe\source\repos\Bracketext\Bracketext\macros.txt");
            bb.Init();
            /*
            {
                //bb.ScanFile(CommandLine["f"]);
                // bb.ScanFile(@"C:\Users\prefe\source\repos\BBMacro\BBMacro\bbcodeexample.txt");
                bb.BBCodeToTree();
                bb.EvalTree();
                // Console.Write(bb.DocumentToHTML());
                //TextWriter txt = new StreamWriter(CommandLine["o"]);
                txt.Write(bb.DocumentToHTML());
                txt.Close();
            }
            */
            
            bb.ScanFile(@"C:\Users\prefe\source\repos\Bracketext\Bracketext\bbcodeexample.txt");
            bb.BBCodeToTree();
            bb.EvalTree();
            Console.Write(bb.DocumentToHTML());
            Console.Read();
            
        }
    }
}
