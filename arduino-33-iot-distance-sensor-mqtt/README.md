# Use Case 
The distance sensor checks if there is a bottle placed properly in the Milquino. This state is then send to [Home Assistant (HA)](https://www.home-assistant.io/) via MQTT (part of HA).

Together with the [other scripts](../milquino/) and a nice button from [Flic](https://flic.io/), this script can prevent you from making a mistake when triggering the milk process: You forgot to place the bottle.

This is done by checking the state of the bottle in HA before the action to send the milk command to Milquino.
# Hardware
This software runs on an Ardunio 33 IOT with a short range distance sensor. I do not remember which sensor I used for this, but it was nothing special. The Ardunio connect to Wifi.
# Software
This is a state machine, see the loop() method. It makes sure it is connected to Wifi and then reads the sensor in a loop. If Wifi breaks it will reconnect automatically. Only if the sensor value crosses the threshold for a certain amount of time the signal is send via MQTT.

Please create a credentials.h by copying the template and adjusting the values.

There are some dependencies to Arduino, like the MQTT client and the Wifi client.