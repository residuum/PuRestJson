#! /usr/bin/python

import unittest
from pdStarter import runPd

class UrlparamsTests(unittest.TestCase):
    def test_add_single_value(self):
        out = runPd('urlparams/urlparams-add-single-value.pd')
        self.assertEquals(out, '''list key=value;
''')

    def test_add_two_values(self):
        out = runPd('urlparams/urlparams-add-two-values.pd')
        self.assertEquals(out, '''list key=value&key2=value2;
''')

    def test_replace_value(self):
        out = runPd('urlparams/urlparams-replace-value.pd')
        self.assertEquals(out, '''list key=other;
''')

    def test_clear(self):
        out = runPd('urlparams/urlparams-clear.pd')
        self.assertEquals(out, '''list ;
''')

    def test_escape_value(self):
        out = runPd('urlparams/urlparams-escape-value.pd')
        self.assertEquals(out, '''list key=value%20with%20%23spaces%26other%2bst%c3%bcff%20incl.%20%c3%9cml%c3%a4ute;
''')

    def test_escape_key(self):
        out = runPd('urlparams/urlparams-escape-key.pd')
        self.assertEquals(out, '''list key%23%24=value;
''')

if __name__ == '__main__': 
    unittest.main()
