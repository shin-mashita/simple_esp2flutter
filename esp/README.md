# SmartCane

Simple ESP32 device that detects distance at the cane front and detects fall using an accelerometer

## Obstacle detection
Distance is calculated by sending a signal in HC-SR04 and measuring the delay of the received signal. An obstacle is detected if distance calculated is less than 60cm. This can be set in code using the `DISTANCE_THRESHOLD_CM` macro. 

## Fall detection
Fall is detected using a Simple Moving Average (SMA) method. Accelerometer readings are saved into three arrays containing 50 elements each, one for each axis (x-axis, y-axis, z-axis). Each axis have their own resting accelerometer readings. Sensor calibrations result to the resting accelerometer readings below:

$$G_X = 10.39 m/s^2$$
$$G_Y = 9.66 m/s^2$$
$$G_Z = 12.07 m/s^2$$

where $G_X$, $G_Y$ and $G_Z$ the resting acceleration for the x-, y-, and z-axis, respectively.

The arrays follow a dequeue data structure, which means that if the array is full, new array reading will remove the oldest one (First In, First Out). For each array, the average of the first 49 elements is obtained and compared with the latest reading. If the difference is more than the value of the threshold multiplier to the resting accelerations $G_T$, a fall event is detected. For x-axis, the fall event is given by

$$ \bigg|g_{50}-\sum^{49}_{n=1}\frac{g_n}{49}\bigg| > G_T G_X $$

or

$$ \bigg|g_{50}-\frac{g_1 + g_2 + g_3 +...+g_{49}}{49}\bigg| > G_T G_X $$
