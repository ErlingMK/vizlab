# vizlab
Visual studio 2019 setup:

1. Open project properties.
2. Go to C/C++ -> General, and add the path to the Include folder in the Spinnaker SDK to the Additional Include Directories.
3. Go to Linker -> Input, and add all required .lib files from the Spinnaker SDK to Additional Dependencies.
4. Go to VC++ Directories, and add the path to the folder containing the .lib files to Library Dependencies.
5. Might need a restart.
