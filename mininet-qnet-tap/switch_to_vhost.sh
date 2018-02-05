export PS1="[\u@mininet:$1 at \h; \W]\$ "
nsenter -n -t $(ps ax|grep mininet:$1|grep bash|sed 's/ pts.*//')
