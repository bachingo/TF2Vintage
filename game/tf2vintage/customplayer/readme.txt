The customplayer folder is an all new folder only found on TF2V 
that allows you to modify TF2V assets without using the custom folder
or any of the loose files within the TF2V base.

The folder is broken down into three sections, a jingle, a spray, and an items folder.

jingle:
A jingle is a type of soundspray that makes a custom sound 
when the proper button (o by default) is pressed.

Jingles must be 512kb or less, saved as a .WAV file, 
a sample rate of 44000Hz or less, and with the name of jingle.
These are placed as jingle/sound/player/jingle.wav and changed on boot of TF2V.

spray:
A spray is a logo or image placable on a wall
when the proper button (t by default) is pressed.

Sprays must be converted into a .VTF file, with the name of spray.
Fancier sprays such as an animated one must also include a .VMT file as well.
These are placed as materials/vgui/logos/spray.vtf and change on boot of TF2V.

items:
The items schema configures the usable items in TF2V, such as the inventory.
This allows you to add, remove, or modify items used on your
Singleplayer, Listen, or Dedicated Server.

This is saved as a plaintext .TXT file, as scripts/items/items_game.txt 
and changes on boot of TF2V.