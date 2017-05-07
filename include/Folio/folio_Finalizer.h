/*
 * folio_Finalizer.h
 *
 *  Created on: May 6, 2017
 *      Author: marc
 */

#ifndef INCLUDE_FOLIO_FOLIO_FINALIZER_H_
#define INCLUDE_FOLIO_FOLIO_FINALIZER_H_

/**
 * The finalizer, if set, is called when the last reference to
 * the memory is released.
 */
typedef void (*Finalizer)(void *memory);


#endif /* INCLUDE_FOLIO_FOLIO_FINALIZER_H_ */
