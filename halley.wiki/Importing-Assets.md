# The import stage
Before building the project, you'll need to import assets at least once. This is because codegen is part of asset importing, and codegen needs to be executed before the project is run.

When assets are imported, they are read from **/assets_src**, processed, and output to **/assets**. Likewise, codegen source is read from **/gen_src** and output to **/gen**. As such, make sure that **/assets** and **/gen** are **.gitignore**d.

Assets on Halley's **/shared_assets** folder are ALSO imported into every project. This contains files such as default materials and shaders.

Importing assets can be done in one of two ways:

## Halley Editor
A graphical editor (which is, at the time of writing, quite barebones) can be launched and left running. It will automatically detect asset changes and re-import them as needed. On Windows, it will be called **halley-editor.exe**, and it should be run as:

`halley-editor.exe /full/path/to/game/repo/root`

## Halley Command Line Tool
A CLI tool is also available. On Windows, it's called **halley-cmd.exe**, and it's suitable for batch files and build systems. To run it:

`halley-cmd.exe import /full/path/to/game/repo/root /full/path/to/halley/repo/root`