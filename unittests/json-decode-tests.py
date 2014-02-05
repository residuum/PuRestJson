#! /usr/bin/python

import unittest
import os.path
from pdStarter import runPd

class JsonDecodeTests(unittest.TestCase):
    basePath = 'json-decode'
    def test_simple_object(self):
        out = runPd(os.path.join(self.basePath, 'json-decode-simple-object.pd'))
        self.assertEquals(out, '''list key value;
bang;
''')

    def test_multiple_members(self):
        out = runPd(os.path.join(self.basePath, 'json-decode-multiple-members.pd'))
        self.assertEquals(out, '''list key value;
list key2 2;
list key3 0.1;
bang;
''')

if __name__ == '__main__': 
    unittest.main()
