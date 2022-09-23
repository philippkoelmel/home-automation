# Some Home Automation Stuff
This is a collection of stuff I build over the last 2 years for automating some annoying tasks at home.

[Home Asstiatnt](https://www.home-assistant.io/) is the tool of my choice. With an activated MQTT server.
# Milquino
We are blessed with twins. This means a lot of milk bottles during the night. There is an amazing machine to prepare those milk bottles: [Milquino](https://www.milquino.com/)

But Milquino is not build for scale and industrial use. So I had to do some modifications. Milquino offers a version with Wifi and App. That is the one we bought. And we have two of those Milquinos at home, just to deal with the volume during the first years of our kids.

The modifications are:
* [Trigger via button](./milquino/) - An App is nice, but too complicated at 3am. Replace this with a physical button right next to the bed
* [Bottle detector](./arduino-33-iot-distance-sensor-mqtt/) Once this is done, you can make a big mistake: Trigger Milquino to fill the bottle, but you forgot to put the bottle there => A big mess! This needs a fix, so that Milquino only triggers if there is a bottle -> [Watch this in action on Youtube](https://www.youtube.com/shorts/UvMnk4tY--c)

I also experimented with some other mods, e.g. a sensor if the bottle is full. It happened a few times that I accidently triggered Milquino again and because the bottle is already full the milk spilled over. I did this via a weight sensor, but it never went into production.

Another modification was a 3d-printed new part, so that there are guiding rails for placing the bottle. Sometimes it happened that the bottle was not perfectly centered and that lead to over spill.

# Garbage Cans
# Use Case
I don't like predictable days. I love it, if my day start and I do not know what will happen. This puts me add odds with the concept of repeating calendar entries. Especially if this super passive work. And the best example for this is bringing the garbage cans to the street in front of our house.

So you can image that I very reliably just forget this. No reminder, todo system, calendar, alarm clock helps.

I need a solution exactly matching my needs.

So I build it:
* Download the calendar from the local garbage company every month
* Create or update the calendar entries
* Use Home Assistant to check for the calendar events
* HA checks if the garbage can is still at its place
* If it is still there, send me a message via discord
* Keep repeating this, until the gargabe can is no longer at that place, assuming I have brought it to the front of the house

This means on a normal Thursday I receive messages in discord at 6pm every 30 minutes. Then I fulfill my duty and the messages stop. => This works, I no longer forget the garbage cans.

Multiply this with four different kinds of gargabe cans and you get the picture.

# Hardware
In order to detect if the garbage can is at its place in the backyard of the house, I have added a bluetooth beacon to the can and an arduino to scan for those beacons. If it find the beacon the can is assumed to be in the backyard.

This introduces the problem of power. The beacon needs power to send the signal every now and then. I was able to place the Arduino indoors, so no problem there. I "solved" the power problem by adding a battery and charge it via a solar panel that I installed ontop of the garbage can, luckily they are exposed to sunlight most of the day. And of course during winter I would have to remove the snow from the cans, but that is just a small additional action to the usual snow cleaning duties, so no problem.

What I did not solve was the energy consumption. I tried a lot of beacons, from Arduino to NRF chips directly. I did not manage a proper power household. That was eventually the reason I aborted the project.

Maybe there is a commmercial off the shelv bluetooth beacon that fulfills my needs, but I could not find an appropriate(!) one. There are many out there, but for reasons they were not appropriate.

# Software
Again everything is a state machine, from beacon to beacon-scanner. Eventually I settled with NRF52something. But I lost the code for that.

The beacon scanner also retrieves the battery level from the beacons, so that I can track the consumption and analyse my improvements. Or better "improvements", did not actually get better here.

If think this is a good starting point, the beacon scanner: [arduino-33-iot-ble-sensor](./arduino-33-iot-ble-sensor)

Some code for the NRF, really rough stuff, but all that I have left: [nrf_ble_app_beacon](./nrf_ble_app_beacon/)