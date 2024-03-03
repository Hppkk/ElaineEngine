# Elaine Engine

-----------------

Welcome to the Elaine Engine source code!

You can build the Elaine Editor for Windows(Other platforms will be gradually opened in the future); compile Elaine Engine games for target platforms. Modify the code in any way you can imagine, and share your changes with others.

# Branch

-----------------------

Currently only the master branch is open.

# Getting  and running

------------------------

The steps below take you through cloning your own private fork, then compiling and running the editor yourself:

Elanie Engine can be easily build by cmake. Make sure your cmake version is 3.26 or higher.

## Windows

1. Download Git on your computer and use the following commond:
   
   ```
   git clone --recursive https://github.com/Hppkk/ElaineEngine.git
   ```
   
   When updating existing repository, don't forget to update all submodules:
   
   ```
   git pull
   git submodule init
   git submodule update --recursive
   ```

2. Install Visual Studio 2022 or Visual Studio 2019. 

3. Run the batch file `setup_windows.bat`. That will generate the VS solution in the path `build`. Open the `ElanieEngine.sln`with VS to run the project. Now you can start creating your own.

4. Choose one config between EditorDebug or EditorRelease  to debug your Editor.

5. Choose one config between RuntimeDebug or RuntimeRelease to debug your Game Client.

# Features

-----------------------------

1. The code in the Elaine Engine is written by C++20. 

2. Elaine Engine Core using the Vulkan and DX12 to rendering, you can see the source code in the core layer.

3. We use the [Geometric Tools](http://www.geometrictools.com/) for our math library.

# Contributions

---------------------------

We welcome contributions to ElaineEngine development through pull requests on GitHub.