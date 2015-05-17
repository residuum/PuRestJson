#!/usr/bin/python

from grip import export
from os import listdir, rename
from os.path import splitext

dirName = "../PuRestJson.wiki/"
exportDir = "manual/"

for fileName in listdir(dirName):
    if fileName.endswith(".md"):
        export(dirName + fileName, False, None, None, None, True, True, False, exportDir + splitext(fileName)[0] + ".html")


rename(exportDir + "Home.html", exportDir + "index.html")
