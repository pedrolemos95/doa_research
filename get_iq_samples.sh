# Check arguments
if [ $# -lt 4 ]
then
    printf "Usage $0 <receiver_mac_address> <tag_mac_address> <network_layout_file> <receiver_port>\n"
    exit 1
fi

DATE_WITH_TIME=`date "+%Y-%m-%d-%H:%M:%S"`
OUTPUT_FILE=iq_samples/iq_samples_$DATE_WITH_TIME.json

receiver_mac_address=$1
tag_mac_address=$2
network_layout_file=$3
receiver_port=$4

mqtt_topic=silabs/aoa/iq_report/ble-pd-$receiver_mac_address/ble-pd-$tag_mac_address

# Append METADATA to IQ samples file
cat $network_layout_file > $OUTPUT_FILE

# Store IQ samples inside OUTPUT_FILE
mosquitto_sub -h localhost -p 1883 -t $mqtt_topic >> $OUTPUT_FILE &

receiver/exe/bt_aoa_host_locator -c receiver/config/locator_config.json -u $receiver_port
