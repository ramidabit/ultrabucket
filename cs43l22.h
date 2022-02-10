// This code comes from https://github.com/deater/vmw-meter/tree/master/stm32L476/chiptune_cs43l22

#define CS43L22_REG_ID			0x01
#define CS43L22_REG_POWER_CTL1		0x02
#define CS43L22_REG_POWER_CTL2		0x04
#define CS43L22_REG_CLOCKING_CTL	0x05
#define	CS43L22_REG_INTERFACE_CTL1	0x06

#define	CS43L22_REG_ANALOG_ZC_SR_SETT	0x0A
#define	CS43L22_REG_PLAYBACK_CTL1	0x0D

#define	CS43L22_REG_MISC_CTL		0x0E

#define	CS43L22_REG_PCMA_VOL		0x1A
#define	CS43L22_REG_PCMB_VOL		0x1B
#define	CS43L22_REG_BEEP_FREQ_ON_TIME	0x1C
#define	CS43L22_REG_BEEP_VOL_OFF_TIME	0x1D
#define	CS43L22_REG_BEEP_TONE_CFG	0x1E
#define	CS43L22_REG_TONE_CTL		0x1F
#define	CS43L22_REG_MASTER_A_VOL	0x20
#define	CS43L22_REG_MASTER_B_VOL	0x21
#define	CS43L22_REG_HEADPHONE_A_VOL	0x22
#define	CS43L22_REG_HEADPHONE_B_VOL	0x23
#define	CS43L22_REG_SPEAKER_A_VOL	0x24
#define	CS43L22_REG_SPEAKER_B_VOL	0x25
#define	CS43L22_REG_LIMIT_CTL1		0x27

#define	CS43L22_REG_TEMPMONITOR_CTL	0x32

void cs43l22_init(void);
void cs43l22_disable(void);
void cs43l22_play(void);
void cs43l22_set_volume(int volume);
void cs43l22_beep(void);