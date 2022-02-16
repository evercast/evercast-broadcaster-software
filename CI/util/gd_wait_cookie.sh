while true ; do
    curl -c ./cookie -s -L "https://drive.google.com/uc?export=download&id=${fileid}"
    cookie=`cat cookie | grep google`
    if [ "$cookie" != "" ]; then
        break
    fi
    echo "waiting cookie..."
    sleep 1
done




