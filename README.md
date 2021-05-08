# BUI Project Template

This is an Unreal Engine 4 project template that I have created for starting
small projects as quickly as possible.

It has the following features:

* As many plugins disabled as possible.
* Automated test blanks setup.
* Sensible config defaults.
* ImGUI plugin installed.
* Custom cheat manager.

## Setup

1. Clone this repo somewhere.
2. Update the Git Submodules: `git submodule init` and `git submodule update`
3. Make a symlink from within the Templates directory of your UE4
   install to your Git repo. For example: `mklink /D "C:\Program Files\Epic Games\UE_4.26\Templates\BUIProjectTemplate" "C:\GitStuff\BUIProjectTemplate"` 
4. Run the Unreal New Project Wizard and choose "BUI Blank Project"

## Getting Updates

### Updating submodules

1. `cd Plugins\BUIValidator`
2. `git fetch`
2. `git merge origin/master`
