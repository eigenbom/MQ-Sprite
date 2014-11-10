MMpixel
=======

MMPixel is a tool for creating low resolution game sprites, developed by Ben Porter (<a href="https//twitter.com/eigenbom">@eigenbom</a>) to create the sprites for his game, Moonman.

At a basic level it allows a user to draw frames for static or animated sprites, and then join them together with anchors and pivots. It is an all-in-one editor, containing pixel editting tools and animation tools. Extra sprite data, such as bounding boxes can also be specified.

MMPixel stores a project file as a .tar file containing sprite frames as .pngs and data as .json. Therefore it is relatively easy to create tools or scripts to process the data into your game engine format. For example, in Moonman I wrote a python script that collects all the frames into a single sprite sheet and converts the sprite data into a Moonman-friendly format.

MMPixel is primarily for _low resolution sprites_, on the order of 32x32 pixels. Although it can be used for higher resolution sprites, this isn't the focus and so it may run suboptimally. Tools like Spriter or Spine are much more suited towards hi-res sprite animation.

build instructions
==================
MMPixel is currently undergoing some restructuring and so hence the data format will change over the next few months. However, if you're interested in checking it out, then you will require Qt 5.3 (from http://qt-project.org/) and the easiest way to build it is to use QtCreator. Good luck!

credits
=======
I use some icons from
http://www.gentleface.com/free_icon_set.html#geticons.
