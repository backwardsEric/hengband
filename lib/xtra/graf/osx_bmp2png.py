"""
Converts a tile set from bmp to png for use with the Mac OS X frontend.

Usage
    python osx_bmp2png.py input_file output_file [options]

Options
--mask maskfile
    Reads the mask (zero for transparent pixels; non-zero for opaque pixels)
    from the bmp file, maskfile.  maskfile must have the same x and y
    dimensions as input_file but need not match the color space or bit depth.
    --mask overrides --tcoord, --terrain, and --transparent.

--tcoord hcoord vcoord
    Reads the color to be treated as transparent for non-terrain tiles
    from the position (hcoord, vcoord) in the tile set.  Both coordinates
    are non-negative integers indexed from zero.  Is overridden by
    --transparent or --mask if either of those options are set.  When not
    set, the default is to read the color to be treated as transparent from
    (0,0).

--terrain lx1 ly1 ux1 uy1 ...
    Sets bounding boxes for parts of the tile set that are to be treated
    as completely opaque (i.e. as background terrain).  Each bound box is
    given by the coordinates for the lower corner, (lx,ly), and upper
    corner, (ux,uy), with ux > lx and uy > ly.  If not set, the default
    is to use seven bounding boxes, (0,0) to (256,16), (64,16) to
    (256,24), (216,80) to (240,88), (48,272) to (152,280), (176,272) to
    (192,280), (0,632) to (256,688), and (0,712) to (256,736) which are the
    terrain tiles in Hengband's old 8x8 tile set.  Is overridden by the
    --mask option if that option is set.

--transparent red green blue
    Sets the color, as RGB values between 0 and 255, which will be treated
    as completely transparent for non-terrain tiles.  If not set, uses
    --tcoord or its default to get the color.  Is overridden by the --mask
    option if that option is set.

Example
This is the conversion that was used to prepare the old tile set for the
OS X front end:
    python osx_bmp2png.py 8x8.bmp 8x8.png
That is equivalent to
    python osx_bmp2png.py 8x8.bmp 8x8.png -tcoord 0 0 \
        -terrain 0 0 256 16 64 16 256 24 216 80 240 88 48 272 152 280 \
        176 272 192 280 0 632 256 688 0 712 256 736

This is the conversion that was used to prepare Adam Bolt's tile set for the
OS X front end:
    python osx_bmp2png.py 16x16.bmp 16x16.png --mask 16x16-mask.bmp

Requirements
argparse, PIL (Python imaging library) or equivalent, and
numpy (1.7 or later)

The conversions are required because CGImageSourceCreateWithURL() does not
handle the bmp format.
"""

import argparse
import PIL.Image
import numpy

aparser = argparse.ArgumentParser(description='Converts bmp tile set to png.')
aparser.add_argument('input_file', type=str,
        help='the path to the input tile set')
aparser.add_argument('output_file', type=str,
        help='the path to the converted tile set')
aparser.add_argument('--mask', default='',
        help='File name for BMP file with transparency mask')
aparser.add_argument('--tcoord', type=int, nargs=2, default=(0,32),
        help='Position to be read for transparent color')
aparser.add_argument('--terrain', type=int, nargs='+',
        default=(0,0,256,16,64,16,256,24,216,80,240,88,48,272,152,280,176,272,192,280,0,632,256,688,0,712,256,736),
        help='Bounding box coordinates for terrain tiles')
aparser.add_argument('--transparent', type=int, nargs=3,
        help='RGB value to be treated as transparent')

args = aparser.parse_args()

tin = PIL.Image.open(args.input_file)

if args.mask == '' and len(args.terrain) % 4 != 0:
        raise IndexError('Need four values for each bounding box.')
for i in range(0, len(args.terrain), 4):
        if (args.terrain[i] > args.terrain[i + 2] or
                        args.terrain[i + 1] > args.terrain[i + 3]):
                raise RuntimeError('Lower bounding box coordinate exceeds the upper one')
        if (args.terrain[i] < 0 or args.terrain[i + 2] > tin.width or
                        args.terrain[i + 1] < 0 or
                        args.terrain[i + 3] > tin.height):
                raise IndexError('Bounding box outside of image bounds')

palette = tin.getpalette()

if args.mask == '':
        if args.transparent:
                trind = None
                trarr = numpy.array(args.transparent)
        else:
                if (args.tcoord[0] < 0 or args.tcoord[0] >= tin.width or
                                args.tcoord[1] < 0 or
                                args.tcoord[1] >= tin.height):
                        raise IndexError('Coordinates for transparent pixel are out of bounds')
                if palette:
                        trind = tin.getpixel(tuple(args.tcoord))
                else:
                        trarr = numpy.array(tin.getpixel(args.tcoord)[0:3])

arrin = numpy.array(tin)

# Copy over the RGB components.
arrout = numpy.empty((tin.height,tin.width,4), dtype='u1')
if palette:
        parr = numpy.reshape(numpy.array(palette), (len(palette) // 3, 3))
        arrout[:,:,0:3] = parr[arrin]
else:
        arrout[:,:,0:3] = arrin[:,:,0:3]


# Set up the alpha channel.
if args.mask == '':
        # Set up mask for portions of the data set not containing terrain.
        arrt = numpy.ones((tin.height, tin.width), dtype=numpy.dtype(bool))
        ind0, ind1 = numpy.meshgrid(numpy.linspace(0, tin.height - 1, tin.height),
                numpy.linspace(0, tin.width - 1, tin.width), indexing='ij')
        for i in range(0, len(args.terrain), 4):
                arrt = numpy.logical_and(arrt,
                        numpy.logical_or(numpy.logical_or(
                        ind1 < args.terrain[i], ind0 < args.terrain[i + 1]),
                        numpy.logical_or(ind1 >= args.terrain[i + 2],
                        ind0 >= args.terrain[i + 3])))
        del ind0, ind1

        # Combine with where the input is equal to transparent color to get
        # the transparency mask.
        if palette:
                if trind is not None:
                        arrout[:,:,3] = numpy.where(
                                numpy.logical_and(arrt, arrin == trind),
                                numpy.zeros((tin.height, tin.width), dtype='u1'),
                                255 * numpy.ones((tin.height, tin.width), dtype='u1'))
                else:
                        arrout[:,:,3] = numpy.where(
                                numpy.logical_and(arrt, numpy.logical_and(
                                arrout[:,:,0] == trarr[0], numpy.logical_and(
                                arrout[:,:,1] == trarr[1], arrout[:,:,2] == trarr[2]))),
                                numpy.zeros((tin.height, tin.width), dtype='u1'),
                                255 * numpy.ones((tin.height, tin.width), dtype='u1'))
        else:
                arrout[:,:,3] = numpy.where(
                        numpy.logical_and(arrt, numpy.logical_and(
                        arrin[:,:,0] == trarr[0], numpy.logical_and(
                        arrin[:,:,1] == trarr[1], arrin[:,:,2] == trarr[2]))),
                        numpy.zeros((tin.height, tin.width), dtype='u1'),
                        255 * numpy.ones((tin.height, tin.width), dtype='u1'))
        del arrt

else:
        tmask = PIL.Image.open(args.mask)
        if tmask.height != tin.height or tmask.width != tin.width:
                raise RuntimeError('Dimensions of mask do not match input dimensions')
        arrmask = numpy.array(tmask)
        if tmask.getpalette():
                arrout[:,:,3] = numpy.where(arrmask == 0,
                        numpy.zeros((tin.height, tin.width), dtype='u1'),
                        255 * numpy.ones((tin.height, tin.width), dtype='u1'))
        else:
                arrout[:,:,3] = numpy.where(
                        numpy.logical_and(arrmask[:,:,0] == 0,
                        numpy.logical_and(arrmask[:,:,1] == 0,
                        arrmask[:,:,2] == 0)),
                        numpy.zeros((tin.height, tin.width), dtype='u1'),
                        255 * numpy.ones((tin.height, tin.width), dtype='u1'))
        del arrmask, tmask


tout = PIL.Image.fromarray(arrout)
tout.save(args.output_file)
