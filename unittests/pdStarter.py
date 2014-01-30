#! /usr/bin/python

import subprocess

def runPd(pdPatch):
    pdStartup = 'pd'
    pd = subprocess.Popen([pdStartup, '-nosound', '-batch', '-send', 'unittest bang', pdPatch],
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = pd.communicate()
    return out
