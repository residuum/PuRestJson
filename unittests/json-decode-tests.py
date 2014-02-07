#! /usr/bin/python

import unittest
import os.path
from pdStarter import runPd

class JsonDecodeTests(unittest.TestCase):
    basePath = 'json-decode'
    def test_simple_object(self):
        out = runPd(os.path.join(self.basePath, 'simple-object.pd'))
        self.assertEquals(out, '''list key value;
bang;
''')

    def test_multiple_members(self):
        out = runPd(os.path.join(self.basePath, 'multiple-members.pd'))
        self.assertEquals(out, '''list key value;
list key2 2;
list key3 0.1;
list bool 0;
bang;
''')

    def test_array(self):
        out = runPd(os.path.join(self.basePath, 'array.pd'))
        self.assertEquals(out, '''list key value;
list id 1;
bang;
list key value 2;
list id 2;
bang;
''')

if __name__ == '__main__': 
    unittest.main()
