import pybdsim
import ROOT

def Analyse(filename):
    # load data and access the event tree
    d = pybdsim.Data.Load(filename)
    eventTree = d.GetEventTree()
    
    combined     = set()
    creator      = set()

    # loop over events
    for i,event in enumerate(eventTree):
        # loop over trajectories
        for i in range(event.Trajectory.n):
            if abs(event.Trajectory.partID[i]) == 13:
                event.Trajectory.printTrajectoryInfo(i)
                parentTrackIndex    = event.Trajectory.parentIndex[i]
                event.Trajectory.printTrajectoryInfo(parentTrackIndex)
                parentTrajStepIndex = event.Trajectory.parentStepIndex[i]
                print("parent track index: ",parentTrackIndex)
                print("parent step index:  ",parentTrajStepIndex)
                print("n trajectories:     ",event.Trajectory.postProcessTypes.size())
                print("n points on parent: ",event.Trajectory.postProcessTypes[parentTrackIndex].size())
                creatorPostProcessID    = event.Trajectory.postProcessTypes[parentTrackIndex][parentTrajStepIndex]
                creatorPostProcessSubID = event.Trajectory.postProcessSubTypes[parentTrackIndex][parentTrajStepIndex]
                print("creator process     ID ",creatorPostProcessID)
                print("creator process sub ID ",creatorPostProcessSubID)
                processID.add(creatorPostProcessID)
                processSubID.add(creatorPostProcessSubID)
                combined.add((creatorPostProcessID,creatorPostProcessSubID))
                creator.add((event.Trajectory.preProcessTypes[i][0],event.Trajectory.preProcessSubTypes[i][0]))


    print(combined)
    print(creator)
            
