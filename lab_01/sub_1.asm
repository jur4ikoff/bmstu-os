		sub_0001	proc	near
; Сохрание регистров ds, ax
020A:07B9  1E					push	ds
020A:07BA  50					push	ax
; Заполнение регистра ds
020A:07BB  B8 0040				mov	ax,40h
020A:07BE  8E D8				mov	ds,ax
; Запись младшего байта FLAGS в ah
020A:07C0  9F					lahf				; Load ah from flags
; Установлен ли флаг DF или старший бит IOPL по адресу 0040:0314h
020A:07C1  F7 06 0314 2400			test	word ptr ds:[314h],2400h	; (0040:0314=5200h)
020A:07C7  75 0C				jnz	loc_0007		; Jump if not zero
; Сбросить флаг iF по адресу 0040:0314h, с помощью обнуления 9 бита
020A:07C9  F0> 81 26 0314 FDFF	                           lock	and	word ptr ds:[314h],0FDFFh	; (0040:0314=5200h)
020A:07D0			loc_0006::
; Загрузить AH в младший байт FLAGS
020A:07D0  9E					sahf				; Store ah into flags
; Восстановление регистров AX, DS
020A:07D1  58					pop	ax
020A:07D2  1F					pop	ds
020A:07D3  EB 03				jmp	short loc_0008		; (07D8)
020A:07D5			loc_0007::
; Сбросить флаг IF в FLAGS с помощью инструкции cli
020A:07D5  FA					cli				; Disable interrupts
020A:07D6  EB F8				jmp	short loc_0006		; (07D0)
020A:07D8			loc_0008::
020A:07D8  C3					retn
				sub_0001	endp