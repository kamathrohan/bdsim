import pybdsim
import ROOT



def CopyToFlatFile(bdsimFile, outputFileName):

    d = pybdsim.Data.Load(bdsimFile)

    o = ROOT.TFile(outputFileName, "recreate")

    for event in d.GetEventTree():
