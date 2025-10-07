/* generated vector header file - do not edit */
#ifndef VECTOR_DATA_H
#define VECTOR_DATA_H
#ifdef __cplusplus
        extern "C" {
        #endif
/* Number of interrupts allocated */
#ifndef VECTOR_DATA_IRQ_COUNT
#define VECTOR_DATA_IRQ_COUNT    (4)
#endif
/* ISR prototypes */
void iic_b_master_rxi_isr(void);
void iic_b_master_txi_isr(void);
void iic_b_master_tei_isr(void);
void iic_b_master_eri_isr(void);

/* Vector table allocations */
#define VECTOR_NUMBER_IICB0_RXI ((IRQn_Type) 0) /* IICB0 RXI (Receive) */
#define IICB0_RXI_IRQn          ((IRQn_Type) 0) /* IICB0 RXI (Receive) */
#define VECTOR_NUMBER_IICB0_TXI ((IRQn_Type) 1) /* IICB0 TXI (Transmit) */
#define IICB0_TXI_IRQn          ((IRQn_Type) 1) /* IICB0 TXI (Transmit) */
#define VECTOR_NUMBER_IICB0_TEI ((IRQn_Type) 2) /* IICB0 TEI (Transmit end) */
#define IICB0_TEI_IRQn          ((IRQn_Type) 2) /* IICB0 TEI (Transmit end) */
#define VECTOR_NUMBER_IICB0_ERI ((IRQn_Type) 3) /* IICB0 ERI (Error) */
#define IICB0_ERI_IRQn          ((IRQn_Type) 3) /* IICB0 ERI (Error) */
/* The number of entries required for the ICU vector table. */
#define BSP_ICU_VECTOR_NUM_ENTRIES (4)

#ifdef __cplusplus
        }
        #endif
#endif /* VECTOR_DATA_H */
