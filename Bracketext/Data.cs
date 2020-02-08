using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Serialization;

namespace Bracketext
{
    class Data
    {
        public string Serialize(List<string> sList)
        {
            StringWriter sw = new StringWriter();
            XmlSerializer xml = new XmlSerializer(sList.GetType());
            xml.Serialize(sw, sList);
            return sw.ToString();
        }
        public List<string> Deserialize(string str)
        {
            XmlSerializer xml = new XmlSerializer(typeof(List<string>));
            List<string> sList = (List<string>)xml.Deserialize(new StringReader(str));
            return sList;
        }
    }
}
