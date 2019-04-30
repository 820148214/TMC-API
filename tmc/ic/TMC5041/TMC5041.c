/*
 * TMC5041.c
 *
 *  Created on: 07.07.2017
 *      Author: LK
 *    Based on: TMC562-MKL.h (26.01.2012 OK)
 */

#include "TMC5041.h"

// => SPI wrapper
extern void tmc5041_writeDatagram(uint8_t motor, uint8_t address, uint8_t x1, uint8_t x2, uint8_t x3, uint8_t x4);
extern void tmc5041_writeInt(uint8_t motor, uint8_t address, int value);
extern int tmc5041_readInt(uint8_t motor, uint8_t address);
// <= SPI wrapper

void tmc5041_init(TMC5041TypeDef *tmc5041, uint8_t channel, ConfigurationTypeDef *config, const int32_t *registerResetState)
{
	tmc5041->velocity[0]      = 0;
	tmc5041->velocity[1]      = 0;
	tmc5041->oldTick          = 0;
	tmc5041->oldX[0]          = 0;
	tmc5041->oldX[1]          = 0;
	tmc5041->vMaxModified[0]  = false;
	tmc5041->vMaxModified[1]  = false;

	tmc5041->config               = config;
	tmc5041->config->callback     = NULL;
	tmc5041->config->channel      = channel;
	tmc5041->config->configIndex  = 0;
	tmc5041->config->state        = CONFIG_READY;

	int i;
	for(i = 0; i < TMC5041_REGISTER_COUNT; i++)
	{
		tmc5041->registerAccess[i]      = tmc5041_defaultRegisterAccess[i];
		tmc5041->registerResetState[i]  = registerResetState[i];
	}
}

static void tmc5041_writeConfiguration(TMC5041TypeDef *tmc5041)
{
	uint8_t *ptr = &tmc5041->config->configIndex;
	const int32_t *settings = (tmc5041->config->state == CONFIG_RESTORE) ? tmc5041->config->shadowRegister : tmc5041->registerResetState;

	while((*ptr < TMC5041_REGISTER_COUNT) && !TMC_IS_WRITABLE(tmc5041->registerAccess[*ptr]))
		(*ptr)++;

	if(*ptr < TMC5041_REGISTER_COUNT)
	{
		tmc5041_writeInt(0, *ptr, settings[*ptr]);
		(*ptr)++;
	}
	else
	{
		tmc5041->config->state = CONFIG_READY;
	}
}

void tmc5041_periodicJob(TMC5041TypeDef *tmc5041, uint32_t tick)
{
	int xActual;
	uint32_t tickDiff;

	if(tmc5041->config->state != CONFIG_READY)
	{
		tmc5041_writeConfiguration(tmc5041);
		return;
	}

	if((tickDiff = tick - tmc5041->oldTick) >= 5)
	{
		int i;
		for (i = 0; i < TMC5041_MOTORS; i++)
		{
			xActual = tmc5041_readInt(0, TMC5041_XACTUAL(i));
			tmc5041->config->shadowRegister[TMC5041_XACTUAL(i)] = xActual;
			tmc5041->velocity[i] = (int) ((float) (abs(xActual-tmc5041->oldX[i]) / (float) tickDiff) * (float) 1048.576);
			tmc5041->oldX[i] = xActual;
		}
		tmc5041->oldTick = tick;
	}
}

uint8_t tmc5041_reset(TMC5041TypeDef *tmc5041)
{
	if(tmc5041->config->state != CONFIG_READY)
		return 0;

	tmc5041->config->state        = CONFIG_RESET;
	tmc5041->config->configIndex  = 0;

	return 1;
}

uint8_t tmc5041_restore(TMC5041TypeDef *tmc5041)
{
	if(tmc5041->config->state != CONFIG_READY)
		return 0;

	tmc5041->config->state        = CONFIG_RESTORE;
	tmc5041->config->configIndex  = 0;

	return 1;
}
