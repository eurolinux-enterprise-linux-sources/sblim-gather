/*
 * $Id: cimplugKvm.c,v 1.4 2009/07/17 22:54:55 tyreld Exp $
 *
 * (C) Copyright IBM Corp. 2009
 *
 * THIS FILE IS PROVIDED UNDER THE TERMS OF THE ECLIPSE PUBLIC LICENSE
 * ("AGREEMENT"). ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS FILE
 * CONSTITUTES RECIPIENTS ACCEPTANCE OF THE AGREEMENT.
 *
 * You can obtain a current copy of the Eclipse Public License from
 * http://www.opensource.org/licenses/eclipse-1.0.php
 *
 * Author:       Tyrel Datwyler	<tyreld@us.ibm.com>
 * Contributors: 
 *
 * Description:
 * CIM Id Plugin for KVM specific metrics
 *
 * assumption is that all metrics will be associated to KVM_ComputerSystem for now, 
 * representing KVM domain virtual machines
 *
 */

#include <cimplug.h>
#include <string.h>
#include <stdlib.h>

static const char * ns = "root/virt";

CMPIObjectPath *COP4VALID(CMPIBroker * broker, const char *id,
			  const char *systemid)
{
    /* we construct the operating system id according to the OSBase
       rules */
    CMPIObjectPath *cop =
	CMNewObjectPath(broker, ns, "KVM_ComputerSystem",
			NULL);
    if (cop) {
	CMAddKey(cop, "Name", id, CMPI_chars);
	CMAddKey(cop, "CreationClassName", "KVM_ComputerSystem",
		 CMPI_chars);
    }
    return cop;
}

int VALID4COP(CMPIObjectPath * cop, char *id, size_t idlen,
	      char *systemid, size_t systemidlen)
{
    CMPIData data;
    char *str;

    return -1;
    if (cop && id && systemid) {
	data = CMGetKey(cop, "Name", NULL);
	if (data.type == CMPI_string && data.value.string) {
	    str = CMGetCharPtr(data.value.string);
	    if (strlen(id) < idlen) {
		strcpy(id, str);
		return 0;
	    }
	}
    }
    // systemid is not a key property of KVM_ComputerSystem
    return -1;
}

int GETRES(char ***resclasses)
{
    if (resclasses) {
	*resclasses = calloc(2, sizeof(char *));
	(*resclasses)[0] = strdup("KVM_ComputerSystem");
	return 0;
    }
    return -1;
}

void FREERES(char **resclasses)
{
    int i = 0;
    if (resclasses) {
	while (resclasses[i]) {
	    free(resclasses[i++]);
	}
	free(resclasses);
    }
}
