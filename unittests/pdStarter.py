#! /usr/bin/python

import subprocess

def runPd(pdPatch):
    pd = subprocess.Popen(['pd', '-nosound', '-batch', '-send', 'unittest bang', pdPatch],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = pd.communicate()
    return out
