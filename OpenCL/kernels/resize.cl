__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

//#pragma OPENCL EXTENSION cl_amd_printf : enable
//int printf(const char *restrict format, ...);

struct CLImage
{
   uint Width;
   uint Height;
};

//  nearest neighbour interpolation resizing
// ----------------------------------------------------------------------------------
//
__kernel void resize_nn(__read_only image2d_t source, __write_only image2d_t dest, struct CLImage src_img, struct CLImage dst_img, float ratioX, float ratioY)
{
   const int gx = get_global_id(0);
   const int gy = get_global_id(1);
   const int2 pos = { gx, gy };

   if (pos.x >= dst_img.Width || pos.y >= dst_img.Height)
      return;

   float2 srcpos = {(pos.x + 0.4995f) / ratioX, (pos.y + 0.4995f) / ratioY};
   int2 SrcSize = (int2)(src_img.Width, src_img.Height);

   float4 value;

   int2 ipos = convert_int2(srcpos);
   if (ipos.x < 0 || ipos.x >= SrcSize.x || ipos.y < 0 || ipos.y >= SrcSize.y)
      value = 0;
   else
      value = read_imagef(source, sampler, ipos);

   write_imagef(dest, pos, value);
}

//  linear interpolation resizing
// ----------------------------------------------------------------------------------
//
__kernel void resize_linear(__read_only image2d_t source, __write_only image2d_t dest, struct CLImage src_img, struct CLImage dst_img, float ratioX, float ratioY)
{
   const int gx = get_global_id(0);
   const int gy = get_global_id(1);
   const int2 pos = { gx, gy };

   if (pos.x >= dst_img.Width || pos.y >= dst_img.Height)
      return;

   float2 srcpos = {(pos.x + 0.4995f) / ratioX, (pos.y + 0.4995f) / ratioY};
   int2 SrcSize = { (int)src_img.Width, (int)src_img.Height };

   float4 value;

   if ((int)(srcpos.x + .5f) == SrcSize.x)
      srcpos.x = SrcSize.x - 0.5001f;

   if ((int)(srcpos.y + .5f) == SrcSize.y)
      srcpos.y = SrcSize.y - 0.5001f;

   srcpos -= (float2)(0.5f, 0.5f);

   if (srcpos.x < -0.5f || srcpos.x >= SrcSize.x - 1 || srcpos.y < -0.5f || srcpos.y >= SrcSize.y - 1)
      value = 0;

   int x1 = (int)(srcpos.x);
   int x2 = (int)(srcpos.x + 1);
   int y1 = (int)(srcpos.y);
   int y2 = (int)(srcpos.y + 1);

   float factorx1 = 1 - (srcpos.x - x1);
   float factorx2 = 1 - factorx1;
   float factory1 = 1 - (srcpos.y - y1);
   float factory2 = 1 - factory1;

   float4 f1 = factorx1 * factory1;
   float4 f2 = factorx2 * factory1;
   float4 f3 = factorx1 * factory2;
   float4 f4 = factorx2 * factory2;

   const int2 pos0 = { x1, y1 };
   const int2 pos1 = { x2, y1 };
   const int2 pos2 = { x1, y2 };
   const int2 pos3 = { x2, y2 };

   float4 v1 = read_imagef(source, sampler, pos0);
   float4 v2 = read_imagef(source, sampler, pos1);
   float4 v3 = read_imagef(source, sampler, pos2);
   float4 v4 = read_imagef(source, sampler, pos3);
   value =  v1 * f1 + v2 * f2 + v3 * f3 + v4 * f4;

   write_imagef(dest, pos, value);
}


//  bicubic interpolation resizing
// ----------------------------------------------------------------------------------
//
float4 sample_bicubic_border(__read_only image2d_t source, float2 pos, int2 SrcSize)
{
   int2 isrcpos = convert_int2(pos);
   float dx = pos.x - isrcpos.x;
   float dy = pos.y - isrcpos.y;

   float4 C[4] = {0, 0, 0, 0};

   if (isrcpos.x < 0 || isrcpos.x >= SrcSize.x)
      return 0;

   if (isrcpos.y < 0 || isrcpos.y >= SrcSize.y)
      return 0;

   for (int i = 0; i < 4; i++)
   {
      int y = isrcpos.y - 1 + i;
      if (y < 0)
         y = 0;

      if (y >= SrcSize.y)
         y = SrcSize.y - 1;

      int Middle = clamp(isrcpos.x, 0, SrcSize.x - 1);

      const int2 pos0 = { Middle, y };
      float4 center = read_imagef(source, sampler, pos0);

      float4 left = 0, right1 = 0, right2 = 0;
      if (isrcpos.x - 1 >= 0)
      {
         const int2 pos1 = { isrcpos.x - 1, y };
         left = read_imagef(source, sampler, pos1);
      }
      else
      {
         left = center;
      }

      if (isrcpos.x + 1 < SrcSize.x)
      {
         const int2 pos2 = { isrcpos.x + 1, y };
         right1 = read_imagef(source, sampler, pos2);
      }
      else
      {
         right1 = center;
      }

      if (isrcpos.x + 2 < SrcSize.x)
      {
         const int2 pos3 = { isrcpos.x + 2, y };
         right2 = read_imagef(source, sampler, pos3);
      }
      else
      {
         right2 = right1;
      }

      float4 a0 = center;
      float4 d0 = left - a0;
      float4 d2 = right1 - a0;
      float4 d3 = right2 - a0;

      float4 a1 = -1.0f / 3 * d0 + d2 - 1.0f / 6 * d3;
      float4 a2 =  1.0f / 2 * d0 + 1.0f / 2 * d2;
      float4 a3 = -1.0f / 6 * d0 - 1.0f / 2 * d2 + 1.0f / 6 * d3;
      C[i] = a0 + a1 * dx + a2 * dx * dx + a3 * dx * dx * dx;
   }

   float4 d0 = C[0] - C[1];
   float4 d2 = C[2] - C[1];
   float4 d3 = C[3] - C[1];
   float4 a0 = C[1];
   float4 a1 = -1.0f / 3 * d0 + d2 -1.0f / 6 * d3;
   float4 a2 = 1.0f / 2 * d0 + 1.0f / 2 * d2;
   float4 a3 = -1.0f / 6 * d0 - 1.0f / 2 * d2 + 1.0f / 6 * d3;

   return a0 + a1 * dy + a2 * dy * dy + a3 * dy * dy * dy;
}

__kernel void resize_bicubic(__read_only image2d_t source, __write_only image2d_t dest, struct CLImage src_img, struct CLImage dst_img, float ratioX, float ratioY)
{
   const int gx = get_global_id(0);
   const int gy = get_global_id(1);
   const int2 pos = { gx, gy };

   if (pos.x >= dst_img.Width || pos.y >= dst_img.Height)
      return;

   float2 srcpos = {(pos.x + 0.4995f) / ratioX, (pos.y + 0.4995f) / ratioY};
   int2 SrcSize = { (int)src_img.Width, (int)src_img.Height };

   float4 value;

   srcpos -= (float2)(0.5f, 0.5f);

   int2 isrcpos = convert_int2(srcpos);
   float dx = srcpos.x - isrcpos.x;
   float dy = srcpos.y - isrcpos.y;

   if (isrcpos.x <= 0 || isrcpos.x >= SrcSize.x - 2)
      value = sample_bicubic_border(source, srcpos, SrcSize);

   if (isrcpos.y <= 0 || isrcpos.y >= SrcSize.y - 2)
      value = sample_bicubic_border(source, srcpos, SrcSize);

   float4 C[4] = {0, 0, 0, 0};

   for (int i = 0; i < 4; i++)
   {
      const int y = isrcpos.y - 1 + i;

      const int2 pos0 = { isrcpos.x, y };
      const int2 pos1 = { isrcpos.x - 1, y };
      const int2 pos2 = { isrcpos.x + 1, y };
      const int2 pos3 = { isrcpos.x + 2, y };

      float4 a0 = read_imagef(source, sampler, pos0);
      float4 d0 = read_imagef(source, sampler, pos1) - a0;
      float4 d2 = read_imagef(source, sampler, pos2) - a0;
      float4 d3 = read_imagef(source, sampler, pos3) - a0;

      float4 a1 = -1.0f / 3 * d0 + d2 - 1.0f / 6 * d3;
      float4 a2 =  1.0f / 2 * d0 + 1.0f / 2 * d2;
      float4 a3 = -1.0f / 6 * d0 - 1.0f / 2 * d2 + 1.0f / 6 * d3;
      C[i] = a0 + a1 * dx + a2 * dx * dx + a3 * dx * dx * dx;
   }

   float4 d0 = C[0] - C[1];
   float4 d2 = C[2] - C[1];
   float4 d3 = C[3] - C[1];
   float4 a0 = C[1];
   float4 a1 = -1.0f / 3 * d0 + d2 -1.0f / 6 * d3;
   float4 a2 = 1.0f / 2 * d0 + 1.0f / 2 * d2;
   float4 a3 = -1.0f / 6 * d0 - 1.0f / 2 * d2 + 1.0f / 6 * d3;
   value = a0 + a1 * dy + a2 * dy * dy + a3 * dy * dy * dy;

   write_imagef(dest, pos, value);
}
