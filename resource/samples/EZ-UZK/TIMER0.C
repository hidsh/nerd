/*
 * timer0.c : make 1msec tick by timer0
 *   by uratan! 2004.5.24
 */
 	/*
	 * �������̏����Ŏg�� 1msec �̊�p���X�� timer0 �ɂč��܂��B
	 */
#include <ezusb.h>
#include <ezregs.h>

/*
 * function declarations
 */
void init_timer0(void);
void timer0_isr(void); /* interrupt TMR0_VECT using 3 */

/*
 * set when timer0 overflow (every 1msec (COUNT_INITIAL))
 */
BOOL timer0_tick;

/*
 * initial timer0
 */
void init_timer0(void)
{
	/*
	 * use timer0, 24MHz/12 clock, 2000 counts for 1msec
	 */
#define COUNT_INITIAL   (65536-2000)

	CKCON &= ~0x08;		/* T0M=0, clock source is CLK24/12 */
	TMOD &= ~0x0f;		/* clear bits of timer0 */
	TMOD |= 0x01;		/* select mode1 */
	TH0 = MSB(COUNT_INITIAL);
	TL0 = LSB(COUNT_INITIAL);
	TR0 = 1;			/* run timer1 */

	/*
	 * initial my variable
	 */
	timer0_tick = FALSE;

	/*
	 * at last, enable timer0 interrupt
	 */
	ET0 = 1;

	return;
}

/*
 * timer0 Interrupt Service Routine
 */
 	/*
	 * ���荞�݃n���h���̃R�[�h�Z�k�̂��߂Ƀ��W�X�^�o���N���w�肵�Ă��܂��B
	 * ���̃o���N�w��̂��߂ɁA�R���p�C���I�v�V������ NOAREGS ��t�����Ă��܂��B
	 *   (���ۂɂ͂��̃n���h���̋L�q�ł̓��W�X�^���g�킸�ɍς�ł��܂�)
	 * �܂��A���荞�݃x�N�^�̐ݒ���R���p�C���܂����ɂ��Ă��܂��B
	 * �e���荞�݃n���h�����̃��W�X�^���t�́Aperiph.c ���̃R�����g��
	 * �Q�Ƃ���Ȃ� grep ���Ē��ׂ�Ȃ肵�Ă��������B
	 */
void timer0_isr(void) interrupt TMR0_VECT using 3
{
	/*
	 * reload initial count value manually
	 */
	TR0 = 0;			/* stop timer1 */
	TH0 = MSB(COUNT_INITIAL);
	TL0 = LSB(COUNT_INITIAL);
	TR0 = 1;			/* run timer1 */
	timer0_tick = TRUE;
}