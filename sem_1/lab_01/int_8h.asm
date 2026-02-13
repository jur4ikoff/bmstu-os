; Вызов подпрограммы sub_0001
020A:0746  E8 0070				call	sub_0001		; (07B9) 
; Сохранение значений регистров
020A:0749  06					push	es
020A:074A  1E					push	ds
020A:074B  50					push	ax
020A:074C  52					push	dx
; Заполнение регистров ds, es
020A:074D  B8 0040				mov	ax,40h
020A:0750  8E D8				mov	ds,ax
020A:0752  33 C0				xor	ax,ax			; Zero register
020A:0754  8E C0				mov	es,ax
; Инкремент младшего слова таймера
020A:0756  FF 06 006C				inc	word ptr ds:[6Ch]	; (0040:006C=0EF27h)
020A:075A  75 04				jnz	loc_0001		; Jump if not zero
; Инкримент старшего слова таймера
020A:075C  FF 06 006E				inc	word ptr ds:[6Eh]	; (0040:006E=0Eh)
; Проверка прошли ли сутки на таймере
020A:0760			loc_0001::
020A:0760  83 3E 006E 18			cmp	word ptr ds:[6Eh],18h	; (0040:006E=0Eh) Сравнивает содержимое старшего слова таймера с 24
020A:0765  75 15				jne	loc_0002		; Jump if not equal
020A:0767  81 3E 006C 00B0			cmp	word ptr ds:[6Ch],0B0h	; (0040:006C=0EF27h)
020A:076D  75 0D				jne	loc_0002		; Jump if not equal
; Обнуление счетчика таймера
020A:076F  A3 006E				mov	word ptr ds:[6Eh],ax	; (0040:006E=0Eh)
020A:0772  A3 006C				mov	word ptr ds:[6Ch],ax	; (0040:006C=0EF28h)
; Устанавливаем 1 в ячейку 0000:0470h
020A:0775  C6 06 0070 01			mov	byte ptr ds:[70h],1	; (0040:0070=0)
020A:077A  0C 08				or	al,8
; Декремент счетчика времени до выключения моторчика дисковода
020A:077C			loc_0002::
020A:077C  50					push	ax
020A:077D  FE 0E 0040				dec	byte ptr ds:[40h]	; (0040:0040=75h)
020A:0781  75 0B				jnz	loc_0003		; Jump if not zero
; Сбросить флаг моторчика дисковода ; TODOTODOTODOTODO
020A:0783  80 26 003F F0			and	byte ptr ds:[3Fh],0F0h	; (0040:003F=0)
; Отправить команду на отключение моторчика дисковода
020A:0788  B0 0C				mov	al,0Ch
020A:078A  BA 03F2				mov	dx,3F2h
020A:078D  EE					out	dx,al			; port 3F2h, dsk0 contrl output
;
020A:078E			loc_0003::
; Восстановление значение ax
020A:078E  58					pop	ax
; Проверка установлен ли 2 бит (PF) по адресу 0040:0314h
020A:078F  F7 06 0314 0004			test	word ptr ds:[314h],4	; (0040:0314=5200h)
020A:0795  75 0C				jnz	loc_0004		; Jump if not zero
; Загрузка младшего байта FLAGS в ah
020A:0797  9F					lahf				; Load ah from flags
020A:0798  86 E0				xchg	ah,al
; Сохранения значение регистра ax
020A:079A  50					push	ax
; Вызов прерывания 1CH
020A:079B  26: FF 1E 0070			call	dword ptr es:[70h]	; (0000:0070=6ADh)
020A:07A0  EB 03				jmp	short loc_0005		; (07A5)
020A:07A2  90					nop
020A:07A3			loc_0004::
020A:07A3  CD 1C				int	1Ch			; Timer break (call each 18.2ms)
020A:07A5			loc_0005::
; Вызов подпрограммы sub_0001
020A:07A5  E8 0011				call	sub_0001		; (07B9)
; Сброс контроллера прерываний
020A:07A8  B0 20				mov	al,20h			; ' '
020A:07AA  E6 20				out	20h,al			; port 20h, 8259-1 int command
										;  al = 20h, end of interrupt
; Восстановление значения регистров
020A:07AC  5A					pop	dx
020A:07AD  58					pop	ax
020A:07AE  1F					pop	ds
020A:07AF  07					pop	es
020A:07B0  E9 FE99				jmp	$-164h
; ...
; ...
; ...
; Сохранение регистров ds, ax
020A:064C  1E					push	ds
020A:064D  50					push	ax
020A:064E  B8 0040				mov	ax,40h
020A:0651  8E D8				mov	ds,ax
020A:0653  F7 06 0314 2400			test	word ptr ds:[314h],2400h	; (0040:0314=5200h)
020A:0659  75 4F				jnz	loc_8			; Jump if not zero
020A:065B  55					push	bp
020A:065C  8B EC				mov	bp,sp
020A:065E  8B 46 0A				mov	ax,[bp+0Ah]
020A:0661  5D					pop	bp
020A:0662  A9 0100				test	ax,100h
020A:0665  75 43				jnz	loc_8			; Jump if not zero
; ...
; ...
020A:06AA			loc_8:
; Восстанововление регистров ax, ds
020A:06AA  58					pop	ax
020A:06AB  1F					pop	ds
; Завершение прерывания
020A:06AC  CF					iret				; Interrupt return

