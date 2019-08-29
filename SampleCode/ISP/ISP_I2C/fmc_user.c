/***************************************************************************//**
 * @file     fmc_user.c
 * @brief    M2351 series FMC driver source file
 * @version  2.0.0
 *
 * @copyright (C) 2017 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <stdio.h>
#include "fmc_user.h"

#define FMC_BLOCK_SIZE           (FMC_FLASH_PAGE_SIZE * 4UL)


int FMC_Proc(unsigned int u32Cmd, unsigned int addr_start, unsigned int addr_end, unsigned int *data)
{
    unsigned int u32Addr, Reg;

    for (u32Addr = addr_start; u32Addr < addr_end; data++) {
        FMC->ISPCMD = u32Cmd;
        FMC->ISPADDR = u32Addr;

        if (u32Cmd == FMC_ISPCMD_PROGRAM) {
            FMC->ISPDAT = *data;
        }

        FMC->ISPTRG = 0x1;
        __ISB();

        /* Wait ISP cmd complete */
        while (FMC->ISPTRG);

        Reg = FMC->ISPCTL;

        if (Reg & FMC_ISPCTL_ISPFF_Msk) {
            FMC->ISPCTL = Reg;
            return -1;
        }

        if (u32Cmd == FMC_ISPCMD_READ) {
            *data = FMC->ISPDAT;
        }

        if (u32Cmd == FMC_ISPCMD_PAGE_ERASE) {
            u32Addr += FMC_FLASH_PAGE_SIZE;
        } else {
            u32Addr += 4;
        }
    }

    return 0;
}

/**
 * @brief      Program 32-bit data into specified address of flash
 *
 * @param[in]  u32addr  Flash address include APROM, LDROM, Data Flash, and CONFIG
 * @param[in]  u32data  32-bit Data to program
 *
 * @details    To program word data into Flash include APROM, LDROM, Data Flash, and CONFIG.
 *             The corresponding functions in CONFIG are listed in FMC section of TRM.
 *
 * @note
 *             Please make sure that Register Write-Protection Function has been disabled
 *             before using this function. User can check the status of
 *             Register Write-Protection Function with DrvSYS_IsProtectedRegLocked().
 */
int FMC_Write_User(unsigned int u32Addr, unsigned int u32Data)
{
    return FMC_Proc(FMC_ISPCMD_PROGRAM, u32Addr, u32Addr + 4, &u32Data);
}

/**
 * @brief       Read 32-bit Data from specified address of flash
 *
 * @param[in]   u32addr  Flash address include APROM, LDROM, Data Flash, and CONFIG
 *
 * @return      The data of specified address
 *
 * @details     To read word data from Flash include APROM, LDROM, Data Flash, and CONFIG.
 *
 * @note
 *              Please make sure that Register Write-Protection Function has been disabled
 *              before using this function. User can check the status of
 *              Register Write-Protection Function with DrvSYS_IsProtectedRegLocked().
 */
int FMC_Read_User(unsigned int u32Addr, unsigned int *data)
{
    return FMC_Proc(FMC_ISPCMD_READ, u32Addr, u32Addr + 4, data);
}

/**
 * @brief      Flash page erase
 *
 * @param[in]  u32addr  Flash address including APROM, LDROM, Data Flash, and CONFIG
 *
 * @details    To do flash page erase. The target address could be APROM, LDROM, Data Flash, or CONFIG.
 *             The page size is 512 bytes.
 *
 * @note
 *             Please make sure that Register Write-Protection Function has been disabled
 *             before using this function. User can check the status of
 *             Register Write-Protection Function with DrvSYS_IsProtectedRegLocked().
 */
int FMC_Erase_User(unsigned int u32Addr)
{
    return FMC_Proc(FMC_ISPCMD_PAGE_ERASE, u32Addr, u32Addr + 4, 0);
}

void ReadData(unsigned int addr_start, unsigned int addr_end, unsigned int *data)    /* Read data from flash */
{
    FMC_Proc(FMC_ISPCMD_READ, addr_start, addr_end, data);
    return;
}

void WriteData(unsigned int addr_start, unsigned int addr_end, unsigned int *data)  /* Write data into flash */
{
    FMC_Proc(FMC_ISPCMD_PROGRAM, addr_start, addr_end, data);
    return;
}



int EraseAP(unsigned int addr_start, unsigned int size)
{
    unsigned int u32Addr, u32Cmd, u32Size;
    u32Addr = addr_start;

    while (size > 0) {
        if ((size >= FMC_BANK_SIZE) && !(u32Addr & (FMC_BANK_SIZE - 1))) {
            u32Cmd = FMC_ISPCMD_BANK_ERASE;
            u32Size = FMC_BANK_SIZE;
        } else if ((size >= FMC_BLOCK_SIZE) && !(u32Addr & (FMC_BLOCK_SIZE - 1))) {
            u32Cmd = FMC_ISPCMD_BLOCK_ERASE;
            u32Size = FMC_BLOCK_SIZE;
        } else {
            u32Cmd = FMC_ISPCMD_PAGE_ERASE;
            u32Size = FMC_FLASH_PAGE_SIZE;
        }

        FMC->ISPCMD = u32Cmd;
        FMC->ISPADDR = u32Addr;
        FMC->ISPTRG = FMC_ISPTRG_ISPGO_Msk;
        __ISB();

        /* Wait for ISP command done. */
        while (FMC->ISPTRG & FMC_ISPTRG_ISPGO_Msk) ;

        if (FMC->ISPCTL & FMC_ISPCTL_ISPFF_Msk) {
            FMC->ISPCTL |= FMC_ISPCTL_ISPFF_Msk;
            return -1;
        }

        u32Addr += u32Size;
        size -= u32Size;
    }

    return 0;
}

void UpdateConfig(unsigned int *data, unsigned int *res)
{
    FMC_ENABLE_CFG_UPDATE();
    FMC_Proc(FMC_ISPCMD_PAGE_ERASE, CONFIG0, CONFIG0 + 16, 0);
    FMC_Proc(FMC_ISPCMD_PROGRAM, CONFIG0, CONFIG0 + 16, data);

    if (res) {
        FMC_Proc(FMC_ISPCMD_READ, CONFIG0, CONFIG0 + 16, res);
    }

    FMC_DISABLE_CFG_UPDATE();
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
