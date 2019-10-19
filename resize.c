#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "bmp.h"

int main(int argc, char *argv[])
{
    // ensure proper usage
    if (argc != 4)
    {
        fprintf(stderr, "Usage: ./resize float infile outfile\n");
        return 1;
    }

    float scale;
    sscanf(argv[1], "%f", &scale);

    // remember filenames
    char *infile = argv[2];
    char *outfile = argv[3];

    // open input file
    FILE *inptr = fopen(infile, "r");
    if (inptr == NULL)
    {
        fprintf(stderr, "Could not open %s.\n", infile);
        return 2;
    }

    // open output file
    FILE *outptr = fopen(outfile, "w");
    if (outptr == NULL)
    {
        fclose(inptr);
        fprintf(stderr, "Could not create %s.\n", outfile);
        return 3;
    }

    // read infile's BITMAPFILEHEADER
    BITMAPFILEHEADER bf;
    fread(&bf, sizeof(BITMAPFILEHEADER), 1, inptr);

    // read infile's BITMAPINFOHEADER
    BITMAPINFOHEADER bi;
    fread(&bi, sizeof(BITMAPINFOHEADER), 1, inptr);


    int width = bi.biWidth;
    int height = abs(bi.biHeight);
    bi.biWidth = bi.biWidth * scale;
    bi.biHeight = bi.biHeight * scale;

    int paddingNo = bi.biWidth % 4;

    bi.biSizeImage = ((sizeof(RGBTRIPLE) * bi.biWidth) + paddingNo) * abs(bi.biHeight);

    bf.bfSize = bi.biSizeImage + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);


    // ensure infile is (likely) a 24-bit uncompressed BMP 4.0
    if (bf.bfType != 0x4d42 || bf.bfOffBits != 54 || bi.biSize != 40 ||
        bi.biBitCount != 24 || bi.biCompression != 0)
    {
        fclose(outptr);
        fclose(inptr);
        fprintf(stderr, "Unsupported file format.\n");
        return 4;
    }

    // write outfile's BITMAPFILEHEADER
    fwrite(&bf, sizeof(BITMAPFILEHEADER), 1, outptr);

    // write outfile's BITMAPINFOHEADER
    fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, outptr);

    // determine padding for scanlines
    int inputPadding = (4 - (width * sizeof(RGBTRIPLE)) % 4) % 4;
    int outputPadding = (4 - (bi.biWidth * sizeof(RGBTRIPLE)) % 4) % 4;

    int size;

    if (scale < 1)
    {
        scale = round(1 / scale);
	size = (int)(width / floor(scale));
    }
    else
    {
        size = width;
    }
    
    // iterate over infile's scanlines
    for (int i = 0; i < height; i++)
    {
        RGBTRIPLE rgbTriples[size];

        int m = 0;

        // iterate over pixels in scanline
        for (int j = 0; j < width; j++)
        {
            // temporary storage
            RGBTRIPLE triple;

            // read RGB triple from infile
            fread(&triple, sizeof(RGBTRIPLE), 1, inptr);

            // if shrinking image
            if (size != width)
            {
                if (j % (int)(scale) == 0)
                {
                    rgbTriples[m] = triple;
                    m++;
                }
            }
            // enlarging image
            else
            {
                rgbTriples[m] = triple;
                m++;
            }
        }
	// shrinking image
        if (size != width)
        {
            if (i % (int)(scale) == 0)
            {
                for (int k = 0; k < size; k++)
                {
                    fwrite(&rgbTriples[k], sizeof(RGBTRIPLE), 1, outptr);
                }
		//padding
                for (int k = 0; k < outputPadding; k++)
                {
                    fputc(0x00, outptr);
                }
            }
        }
	// enlarging image
        else
        {
            for (int l = 0; l < scale; l++)
            {
                for (int n = 0; n < width; n++)
                {
                    for (int x = 0; x < scale; x++)
                    {
                        fwrite(&rgbTriples[n], sizeof(RGBTRIPLE), 1, outptr);
                    }
                }
		//padding
                for (int k = 0; k < outputPadding; k++)
                {
                    fputc(0x00, outptr);
                }
            }
        }

        // skip over padding, if any
        fseek(inptr, inputPadding, SEEK_CUR);
    }

    // close infile
    fclose(inptr);

    // close outfile
    fclose(outptr);

    // success
    return 0;
}

