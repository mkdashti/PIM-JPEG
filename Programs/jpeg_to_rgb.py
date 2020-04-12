from PIL import Image
im = Image.open('../wall-murals-lale.jpg')

pix = im.load()
im_size = im.size
im_width, im_height = im_size[0], im_size[1]
print("width",im_width)

print("height", im_height)
x=0
y=0
pixel_count =0
bit_count = 0
while y<1:
    x=0
    while x<2:
        cur_pix = pix[x,y]
        r_int = cur_pix[0]
        g_int = cur_pix[1]
        b_int = cur_pix[2]
        
        print(hex(r_int))
        print(hex(g_int))
        print(hex(b_int))
        x +=1
    y += 1
