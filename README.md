# simplepng

Simple Android NDK implementation of png encoder for learning png format.

# Example

~~~cpp
FILE *fp = fopen("test.png", "wb");
FileWriter writer(fp);
// write file header
writer.writeU8Array(HEADER_U8_ARRAY, HEADER_U8_ARRAY_SIZE);
// write IHDR Chunk
PngChunk ihdr_chunk(IHDR);
ihdr_chunk.setData(create_ihdr_bytes(width, height));
writer.writeChunk(ihdr_chunk);
// write IDAT Chunk
PngChunk idat_chunk(IDAT);
idat_chunk.setData(create_idat_bytes(image_data, width, height));
writer.writeChunk(idat_chunk);
// write IEND Chunk
writer.writeU8Array(IEND_U8_ARRAY, IEND_U8_ARRAY_SIZE);
~~~