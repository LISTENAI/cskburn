import sys


def main():
    if len(sys.argv) != 4:
        print("Usage: python bin2c.py [input] [output] [name]")
        sys.exit(-1)

    input_path, output_path, name = sys.argv[1], sys.argv[2], sys.argv[3]

    with open(output_path, "w", newline="\n") as out:
        out.write("#include <stdint.h>\n\n")
        out.write(f"uint8_t {name}[] = {{\n")

        with open(input_path, "rb") as f:
            while True:
                chunk = f.read(16)
                if not chunk:
                    break
                line = ", ".join(f"0x{b:02x}" for b in chunk)
                out.write(f"\t{line},\n")

        out.write("};\n\n")
        out.write(f"uint32_t {name}_len = sizeof({name});\n")


if __name__ == "__main__":
    main()
