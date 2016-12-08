/*
 * Columbia University
 * COMS W4118 Fall 2016
 * Homework 6
 *
 */
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <hardware/hardware.h>
#include <hardware/sensors.h> /* <-- This is a good place to look! */
#include <errno.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include "sensor.h"

static int effective_sensor_light = -1;
static int effective_sensor_prox = -1;
static int effective_sensor_linaccel = -1;

/* helper functions which you should use */
static int open_sensors(struct sensors_module_t **hw_module,
			struct sensors_poll_device_t **poll_device);
static void enumerate_sensors(const struct sensors_module_t *sensors);
static int poll_sensor_data_emulator(void);
static int poll_sensor_data(struct sensor_information *sensor_info,
			    struct sensors_poll_device_t *sensors_device);
static int extract_gps_loc(struct sensor_information *sensor_info);

int main(int argc, char **argv)
{
	struct sensors_module_t *sensors_module = NULL;
	struct sensors_poll_device_t *sensors_device = NULL;
	struct sensor_information sensor_info;
	if (argv[1] && strcmp(argv[1], "-e") == 0)
		goto emulation;

	// TODO: Daemonize

	printf("Opening sensors...\n");
	if (open_sensors(&sensors_module,
			 &sensors_device) < 0) {
		printf("open_sensors failed\n");
		return EXIT_FAILURE;
	}
	enumerate_sensors(sensors_module);
	
	printf("GPS %d\n", sensor_info.microlatitude);
	while (1) {
emulation:
		poll_sensor_data(&sensor_info, sensors_device);
		//extract_gps_loc(&sensor_info);
		syscall(244, &sensor_info);
		sleep(2);
	}

	return EXIT_SUCCESS;
}

static int poll_sensor_data(struct sensor_information *sensor_info,
			    struct sensors_poll_device_t *sensors_device)
{
	float cur_intensity = 0;
	float cur_prox = 0;
	float cur_accelx = 0;
	float cur_accely = 0;
	float cur_accelz = 0;

	if (effective_sensor_light < 0) {
	/* emulation */
		cur_intensity = poll_sensor_data_emulator();
	} else {
		//TODO: Collect all the relevant sensor info here.
		sensors_event_t buffer[128];
		ssize_t count = sensors_device->poll(sensors_device,
			buffer, sizeof(buffer)/sizeof(buffer[0]));
		int i;

		for (i = 0; i < count; ++i) {
			if(buffer[i].sensor == effective_sensor_light)
				cur_intensity = buffer[i].light;
			else if (buffer[i].sensor == effective_sensor_prox)
				cur_prox = buffer[i].distance;
			else if (buffer[i].sensor == effective_sensor_linaccel)
			{
				cur_accelx = buffer[i].acceleration.x;
				cur_accelx = buffer[i].acceleration.y;
				cur_accelx = buffer[i].acceleration.z;
			}
		}
	}

	sensor_info->centilux = (int)(cur_intensity * 100);
	sensor_info->centiproximity = (int) (cur_prox * 100);
	sensor_info->centilinearaccelx = (int) (cur_accelx * 100);
	sensor_info->centilinearaccely = (int) (cur_accely * 100);
	sensor_info->centilinearaccelz = (int) (cur_accelz * 100);
	return 0;
}

static int extract_gps_loc(struct sensor_information *sensor_info)
{
	FILE *fp = NULL;
	double lat;
	double lon;

	fp = fopen(GPS_LOCATION_FILE, "r");
	if (fp == NULL) {
		perror("opening file failed. quitting...");
		return -1;
	}

	fscanf(fp, "%lf", &lat);
	fscanf(fp, "%lf", &lon);

	//TODO: Do something with this lat/lon
	sensor_info->microlatitude = (int) (lat * 1000000);
	sensor_info->microlongitude = (int) (lon * 1000000);

	if (fclose(fp)) {
		perror("closing file failed. quitting...");
		return -1;
	}

	return 0;
}

static int poll_sensor_data_emulator(void)
{
	float cur_intensity;
	FILE *fp = fopen("/data/misc/intensity", "r");

	if (!fp)
		return 0;
	fscanf(fp, "%f", &cur_intensity);
	fclose(fp);
	return cur_intensity;
}




static int open_sensors(struct sensors_module_t **mSensorModule,
			struct sensors_poll_device_t **mSensorDevice)
{
	size_t i;
	const struct sensor_t *list;
	ssize_t count;
	int err = hw_get_module(SENSORS_HARDWARE_MODULE_ID,
			(hw_module_t const **)mSensorModule);

	if (err) {
		printf("couldn't load %s module (%s)",
			SENSORS_HARDWARE_MODULE_ID, strerror(-err));
	}

	if (!*mSensorModule)
		return -1;

	err = sensors_open(&((*mSensorModule)->common), mSensorDevice);

	if (err) {
		printf("couldn't open device for module %s (%s)",
			SENSORS_HARDWARE_MODULE_ID, strerror(-err));
	}

	if (!*mSensorDevice)
		return -1;

	count = (*mSensorModule)->get_sensors_list(
		*mSensorModule, &list);
	for (i = 0; i < (size_t)count; i++)
		(*mSensorDevice)->activate(*mSensorDevice, list[i].handle, 1);
	return 0;
}

static void enumerate_sensors(const struct sensors_module_t *sensors)
{
	int nr, s;
	const struct sensor_t *slist = NULL;

	if (!sensors)
		printf("going to fail\n");

	nr = sensors->get_sensors_list((struct sensors_module_t *)sensors,
					&slist);
	if (nr < 1 || slist == NULL) {
		printf("no sensors!\n");
		return;
	}


	for (s = 0; s < nr; s++) {
		printf("%s (%s) v%d\n\tHandle:%d, type:%d, max:%0.2f, "
			"resolution:%0.2f \n", slist[s].name, slist[s].vendor,
			slist[s].version, slist[s].handle, slist[s].type,
			slist[s].maxRange, slist[s].resolution);

		if (slist[s].type == SENSOR_TYPE_LIGHT)
			effective_sensor_light = slist[s].handle;

		if (slist[s].type == SENSOR_TYPE_PROXIMITY)
			effective_sensor_prox = slist[s].handle;
		if (slist[s].type == SENSOR_TYPE_LINEAR_ACCELERATION)
			effective_sensor_linaccel = slist[s].handle;
	}
}
