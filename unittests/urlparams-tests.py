#! /usr/bin/python

import unittest
import os.path
from pdStarter import runPd

class UrlparamsTests(unittest.TestCase):
    basePath = 'urlparams'
    def test_add_single_value(self):
        out = runPd(os.path.join(self.basePath, 'add-single-value.pd'))
        self.assertEquals(out, '''list key=value;
''')

    def test_add_two_values(self):
        out = runPd(os.path.join(self.basePath, 'add-two-values.pd'))
        self.assertEquals(out, '''list key=value&key2=value2;
''')

    def test_replace_value(self):
        out = runPd(os.path.join(self.basePath, 'replace-value.pd'))
        self.assertEquals(out, '''list key=other;
''')

    def test_clear(self):
        out = runPd(os.path.join(self.basePath, 'clear.pd'))
        self.assertEquals(out, '''list ;
''')

    def test_escape_value(self):
        out = runPd(os.path.join(self.basePath, 'escape-value.pd'))
        self.assertEquals(out, '''list key=value%20with%20%23spaces%26other%2bst%c3%bcff%20incl.%20%c3%9cml%c3%a4ute;
''')

    def test_escape_key(self):
        out = runPd(os.path.join(self.basePath, 'escape-key.pd'))
        self.assertEquals(out, '''list key%23%24=value;
''')

if __name__ == '__main__': 
    unittest.main()
