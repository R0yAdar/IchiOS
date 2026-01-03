void main() {
    while(1) { 
    		asm volatile( "int $0x80" :: "a"(0), "c"(0) : "memory" );
			asm volatile( "int $0x80" :: "a"(1), "c"(0) : "memory" );
	}
}