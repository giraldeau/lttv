# -* sh *-

# This is a simple script that starts lttv with default GUI modules
# Mathieu Desnoyers 15-09-2005

$0.real -m guievents -m guifilter -m guicontrolflow -m guistatistics \
    -m guitracecontrol $*

