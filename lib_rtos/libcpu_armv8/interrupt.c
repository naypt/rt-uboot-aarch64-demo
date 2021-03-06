/*
 * Date           Author       Notes
 * 2018-10-06     ZhaoXiaowei    the first version
 */

#include <rthw.h>
#include <rtthread.h>

#include "cpu_port.h"

#include "gic.h"
#include "soc_config.h"

#define MAX_HANDLERS                SOC_HANDLERS

extern volatile rt_uint8_t rt_interrupt_nest;

/* exception and interrupt handler table */
struct rt_irq_desc isr_table[MAX_HANDLERS];

/* Those varibles will be accessed in ISR, so we need to share them. */
rt_uint64_t rt_interrupt_from_thread;
rt_uint64_t rt_interrupt_to_thread;
rt_uint64_t rt_thread_switch_interrupt_flag;

extern int system_vectors;

void rt_cpu_vector_set_base(rt_uint64_t addr)
{
	rt_hw_set_current_vbar(addr);
}

static void rt_hw_vector_init(void)
{
	RT_ASSERT(((rt_uint64_t)&system_vectors & 0x7FF) == 0);
    rt_cpu_vector_set_base((rt_uint64_t)&system_vectors);
}

/**
 * This function will initialize hardware interrupt
 */
void rt_hw_interrupt_init(void)
{
    rt_uint64_t gic_cpu_base;
    rt_uint64_t gic_dist_base;

    /* initialize vector table */
    rt_hw_vector_init();

    /* initialize exceptions table */
    rt_memset(isr_table, 0x00, sizeof(isr_table));

    /* initialize ARM GIC */
    gic_dist_base = SOC_GIC_DIST_BASE;
    gic_cpu_base = SOC_GIC_CPU_BASE;

    arm_gic_dist_init(0, gic_dist_base, 0);
    arm_gic_cpu_init(0, gic_cpu_base);

    /* init interrupt nest, and context in thread sp */
    rt_interrupt_nest = 0;
    rt_interrupt_from_thread = 0;
    rt_interrupt_to_thread = 0;
    rt_thread_switch_interrupt_flag = 0;
}

/**
 * This function will mask a interrupt.
 * @param vector the interrupt number
 */
void rt_hw_interrupt_mask(int vector)
{
    arm_gic_mask(0, vector);
}

/**
 * This function will un-mask a interrupt.
 * @param vector the interrupt number
 */
void rt_hw_interrupt_umask(int vector)
{
    arm_gic_umask(0, vector);
}

/**
 * This function will config a interrupt.
 * @param vector the interrupt number
 * @param config the interrupt config
 */
void rt_hw_interrupt_config(int vector,int config)
{
    arm_gic_config(0, vector, config);
}
/**
 * This function will install a interrupt service routine to a interrupt.
 * @param vector the interrupt number
 * @param new_handler the interrupt service routine to be installed
 * @param old_handler the old interrupt service routine
 */
rt_isr_handler_t rt_hw_interrupt_install(int vector, rt_isr_handler_t handler,
        void *param, char *name)
{
    rt_isr_handler_t old_handler = RT_NULL;

    if (vector < MAX_HANDLERS)
    {
        old_handler = isr_table[vector].handler;

        if (handler != RT_NULL)
        {
#ifdef RT_USING_INTERRUPT_INFO
            rt_strncpy(isr_table[vector].name, name, RT_NAME_MAX);
#endif /* RT_USING_INTERRUPT_INFO */
            isr_table[vector].handler = handler;
            isr_table[vector].param = param;
        }
    }

    return old_handler;
}
