
# Fixes

- [ ] Move scripts/ -> data/scripts/ then move root .sh files to scripts/linux/
- [ ] When a file is modified (in C++ or JS engine), the models must refresh the file's thumbnail. It is the responsibility of the models, not the scripts.
- [ ] Ability to delete a file in display UI.
- [ ] Print the index of the first image on each row of the folder view
- [ ] Press [CTRL-R] on a file, then [->], the focus jumps to the last image in the folder view instead of the next one. Globally, the focus in the folder view MUST NEVER jump to first or last file. If files disappear, focus on the file that's next to it.
- [ ] When in sync mode with folders that have a lot of differences, the selected item displays (sometimes) two overlayed names.
- [ ] Remove flickering when browsing images.
