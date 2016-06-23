#! /usr/bin/python

import unittest
import os.path
from pdStarter import runPd

class JsonEncodeTests(unittest.TestCase):
    basePath = 'json-encode'
    def test_add_string(self):
        out = runPd(os.path.join(self.basePath, 'add-string.pd'))
        self.assertEquals(out, '''list { "key": "value" };
''')

    def test_add_float(self):
        out = runPd(os.path.join(self.basePath, 'add-float.pd'))
        self.assertEquals(out, '''list { "key": 1.100000 };
''')

    def test_replace(self):
        out = runPd(os.path.join(self.basePath, 'replace.pd'))
        self.assertEquals(out, '''list { "key": "value2" };
''')
    
    def test_add_array(self):
        out = runPd(os.path.join(self.basePath, 'add-array.pd'))
        self.assertEquals(out, '''list { "key": [ "value"\\, "value2" ] };
''')
        
    def test_add_object(self):
        out = runPd(os.path.join(self.basePath, 'add-object.pd'))
        self.assertEquals(out, '''list { "key": { "id": "test"\, "name": "my name" } };
''')
        
    def test_clear(self):
        out = runPd(os.path.join(self.basePath, 'clear.pd'))
        self.assertEquals(out, '''list ;
''')
        
    def test_empty_bang(self):
        out = runPd(os.path.join(self.basePath, 'empty-bang.pd'))
        self.assertEquals(out, '''list ;
''')
        
    def test_add_int(self):
        out = runPd(os.path.join(self.basePath, 'add-int.pd'))
        self.assertEquals(out, '''list { "key": 1 };
''')
    
    def test_write(self):
        outFile = os.path.join(self.basePath, 'write.json')
        runPd(os.path.join(self.basePath, 'write.pd'))
        fileHandle = open(outFile)
        out = fileHandle.read();
        try:
            os.remove(outFile);
        except:
            pass
        self.assertEquals(out, '{ "key": 1 }')

    def test_read(self):
        out = runPd(os.path.join(self.basePath, 'read.pd'))
        self.assertEquals(out, '''list { "key": 1 };
''')
    
    def test_read_large(self):
        out = runPd(os.path.join(self.basePath, 'read-large.pd'))
        self.assertEquals(out, '''list { "key": 1 };
''')
    
if __name__ == '__main__': 
    unittest.main()
