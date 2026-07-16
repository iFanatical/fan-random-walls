Small C program to change your wallpaper randomly. Put the compiled binary in $PATH, store your wallpapers in $HOME/Picutres/wallpapers, then call it with a crontab or keybind.  

Compile command: gcc random-walls.c -o random-walls -lX11 -lXinerama -lImlib2  
