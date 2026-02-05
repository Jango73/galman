
# Bugs

- When leaving image display, clicking on an item will not select it, a second click must be done for this. After that, everything works as expected. This happens with the first click just after leaving a display view, in normal and sync mode. It is probably related to the focus not changing on the first click.

- When in sync mode with folders that have a lot of differences, the views have many problems while refreshing :
  - The selected item displays two overlayed names.
  - If there are a lot of files to process (more than 50), we should wait for the folder comparison to finish and display blank views during processing (with a wait icon), and the progress bar active.

# Features

- Ability to add folders as favorites.
