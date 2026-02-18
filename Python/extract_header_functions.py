from pathlib import Path
import re

# ---------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------

INPUT_FILE  = Path("header_function_audit.txt")   # your uploaded audit
OUTPUT_FILE = Path("function_signatures_only.txt")

# Matches the SIGNATURE block exactly as emitted by your extractor
SIGNATURE_RE = re.compile(r"^SIGNATURE:\s*$", re.MULTILINE)

# ---------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------

def normalize_signature(sig: str) -> str:
    """
    Canonicalize a function signature so duplicates collapse.
    """
    sig = sig.rstrip("{").strip()
    sig = " ".join(sig.split())   # collapse whitespace
    return sig


def extract_signatures(text: str):
    lines = text.splitlines()
    signatures = []

    i = 0
    while i < len(lines):
        if lines[i].strip() == "SIGNATURE:":
            if i + 1 < len(lines):
                sig = lines[i + 1].strip()
                signatures.append(sig)
            i += 2
        else:
            i += 1

    return signatures


# ---------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------

def main():
    raw = INPUT_FILE.read_text(encoding="utf-8", errors="ignore")

    raw_signatures = extract_signatures(raw)

    canonical = set()

    for sig in raw_signatures:
        norm = normalize_signature(sig)

        # Drop empty / malformed
        if not norm:
            continue

        # Drop parser artifacts explicitly
        if norm.endswith("{"):
            continue

        canonical.add(norm)

    # Sort for stable diffing and inspection
    ordered = sorted(canonical)

    OUTPUT_FILE.write_text(
        "\n".join(ordered) + "\n",
        encoding="utf-8"
    )

    print(f"Wrote {len(ordered)} canonical function signatures to {OUTPUT_FILE}")


if __name__ == "__main__":
    main()
