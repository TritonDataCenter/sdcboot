#ifndef SORTBOX_H_
#define SORTBOX_H_

struct SortDialogStruct
{
    char SortCriterium;  /* Sort criterium.                           */
    char SortOrder;      /* Sort order.                               */
};

int GetSortOptions(struct SortDialogStruct* sortdialog);

#endif
