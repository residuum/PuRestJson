#! /usr/bin/python

import unittest
from pdStarter import runPd

class JsonEncodeTests(unittest.TestCase):
    basePath = 'json-encode/'
    def test_add_string(self):
        out = runPd(self.basePath + 'json-encode-add-string.pd')
        self.assertEquals(out, '''list { "key": "value" };
''')

    def test_add_float(self):
        out = runPd(self.basePath +'json-encode-add-float.pd')
        self.assertEquals(out, '''list { "key": 1.100000 };
''')

    def test_replace(self):
        out = runPd(self.basePath +'json-encode-replace.pd')
        self.assertEquals(out, '''list { "key": "value2" };
''')
    
    def test_add_array(self):
        out = runPd(self.basePath +'json-encode-add-array.pd')
        self.assertEquals(out, '''list { "key": [ "value"\\, "value2" ] };
''')
        
    def test_add_object(self):
        out = runPd(self.basePath +'json-encode-add-object.pd')
        self.assertEquals(out, '''list { "key": { "id": "test"\, "name": "my name" } };
''')
        
    def test_clear(self):
        out = runPd(self.basePath +'json-encode-clear.pd')
        self.assertEquals(out, '''list ;
''')
        
    def test_empty_bang(self):
        out = runPd(self.basePath +'json-encode-empty-bang.pd')
        self.assertEquals(out, '''list ;
''')
        
    def test_add_int(self):
        out = runPd(self.basePath +'json-encode-add-int.pd')
        self.assertEquals(out, '''list { "key": 1 };
''')

if __name__ == '__main__': 
    unittest.main()
