# analyze_trace.py

import struct
import sys
from collections import defaultdict

RECORD_COMMIT_GRAIN = 1

class TraceAnalyzer:
    def __init__(self, filename):
        self.filename = filename
        self.instruction_counts = defaultdict(int)
        self.opcode_counts = defaultdict(int)
        self.grain_types = defaultdict(int)
        self.missing_grains = []
        
    def analyze(self):
        with open(self.filename, 'rb') as f:
            while True:
                # Read record header
                header = f.read(12)
                if len(header) < 12:
                    break
                    
                record_type, cpu_id, _, timestamp = struct.unpack('<BBHQ', header)
                
                if record_type == RECORD_COMMIT_GRAIN:
                    # Read commit record
                    data = f.read(40)  # Size of CommitRecordWithGrain
                    
                    pc, instr, wcount, flags, opcode, func, gtype, gfound = \
                        struct.unpack('<QIBBBHBB', data[:23])
                    
                    mnemonic = data[23:35].decode('utf-8').rstrip('\x00')
                    
                    # Update statistics
                    self.instruction_counts[mnemonic] += 1
                    self.opcode_counts[opcode] += 1
                    self.grain_types[gtype] += 1
                    
                    if not gfound:
                        self.missing_grains.append({
                            'pc': pc,
                            'opcode': opcode,
                            'function': func,
                            'instr': instr
                        })
    
    def report(self):
        print("="*80)
        print("EXECUTION TRACE ANALYSIS")
        print("="*80)
        print()
        
        total = sum(self.instruction_counts.values())
        print(f"Total Instructions: {total}")
        print()
        
        print("TOP 20 INSTRUCTIONS:")
        print("-"*80)
        for mnem, count in sorted(self.instruction_counts.items(), 
                                  key=lambda x: x[1], reverse=True)[:20]:
            pct = (count * 100.0) / total
            print(f"  {mnem:12s}: {count:10d} ({pct:5.2f}%)")
        print()
        
        print("OPCODES USED:")
        print("-"*80)
        for opcode, count in sorted(self.opcode_counts.items()):
            pct = (count * 100.0) / total
            print(f"  0x{opcode:02X}: {count:10d} ({pct:5.2f}%)")
        print()
        
        if self.missing_grains:
            print(f"  MISSING GRAINS: {len(self.missing_grains)}")
            print("-"*80)
            for mg in self.missing_grains[:10]:  # Show first 10
                print(f"  PC=0x{mg['pc']:016X}, "
                      f"Opcode=0x{mg['opcode']:02X}, "
                      f"Func=0x{mg['function']:04X}, "
                      f"Instr=0x{mg['instr']:08X}")
            if len(self.missing_grains) > 10:
                print(f"  ... and {len(self.missing_grains) - 10} more")

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python analyze_trace.py <trace_file>")
        sys.exit(1)
    
    analyzer = TraceAnalyzer(sys.argv[1])
    analyzer.analyze()
    analyzer.report()