__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

//#pragma OPENCL EXTENSION cl_amd_printf : enable
//int printf(const char *restrict format, ...);

struct SImage
{
   uint Width;
   uint Height;
};

//  nearest neighbour interpolation resizing
// ----------------------------------------------------------------------------------
//
__kernel void resize_nn(__read_only image2d_t source, __write_only image2d_t dest, struct SImage src_img, struct SImage dst_img, float ratioX, float ratioY)
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
__kernel void resize_linear(__read_only image2d_t source, __write_only image2d_t dest, struct SImage src_img, struct SImage dst_img, float ratioX, float ratioY)
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

__kernel void resize_bicubic(__read_only image2d_t source, __write_only image2d_t dest, struct SImage src_img, struct SImage dst_img, float ratioX, float ratioY)
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


//  super-sample interpolation resizing
// ----------------------------------------------------------------------------------
//
float4 supersample_border(__read_only image2d_t source, float2 pos, int2 SrcSize, float2 ratio)
{
   float4 sum = 0;
   float factor_sum = 0;

   float2 start = pos - ratio / 2;
   float2 end = start + ratio;
   int2 istart = convert_int2(start);
   int2 length = convert_int2(end - convert_float2(istart)) + 1;

   float2 factors = 1.f / ratio;

   for (int iy = 0; iy < length.y; iy++)
   {
      int y = istart.y + iy;

      if (y < 0 || y >= SrcSize.y)
         continue;

      float factor_y = factors.y;
      if (y < start.y)
         factor_y = factors.y * (y + 1 - start.y);

      if (y + 1 > end.y)
         factor_y = factors.y * (end.y - y);

      for (int ix = 0; ix < length.x; ix++)
      {
         int x = istart.x + ix;

         if (x < 0 || x >= SrcSize.x)
            continue;

         float factor_x = factors.x;
         if (x < start.x)
            factor_x = factors.x * (x + 1 - start.x);

         if (x + 1 > end.x)
            factor_x = factors.x * (end.x - x);

         float factor = factor_x * factor_y;

         const int2 pos0 = { x, y };

         sum += read_imagef(source, sampler, pos0) * factor;

         factor_sum += factor;
      }
   }

   sum /= factor_sum;

   return sum;
}

kernel void resize_supersample(__read_only image2d_t source, __write_only image2d_t dest, struct SImage src_img, struct SImage dst_img, float ratioX, float ratioY)
{
   const int gx = get_global_id(0);
   const int gy = get_global_id(1);
   const int2 pos = { gx, gy };

   if (pos.x >= dst_img.Width || pos.y >= dst_img.Height)
      return;

   ratioX = 1.0f / ratioX;
   ratioY = 1.0f / ratioY;

   float2 srcpos = {(pos.x + 0.4995f) * ratioX, (pos.y + 0.4995f) * ratioY};
   int2 SrcSize = { (int)src_img.Width, (int)src_img.Height };

   float4 value;

   float2 ratio = (float2)(ratioX, ratioY);

   float4 sum = 0;

   float2 start = srcpos - ratio / 2;
   float2 end = start + ratio;
   int2 istart = convert_int2(start);
   int2 length = convert_int2(end - convert_float2(istart)) + 1;

   float2 factors = 1.f / ratio;

   if (start.x < 0 || start.y < 0 || end.x > SrcSize.x || end.y > SrcSize.y)
      value = supersample_border(source, srcpos, SrcSize, ratio);

   for (int iy = 0; iy < length.y; iy++)
   {
      int y = istart.y + iy;

      float factor_y = factors.y;
      if (y < start.y)
         factor_y = factors.y * (y + 1 - start.y);

      if (y + 1 > end.y)
         factor_y = factors.y * (end.y - y);

      for (int ix = 0; ix < length.x; ix++)
      {
         int x = istart.x + ix;

         float factor_x = factors.x;
         if (x < start.x)
            factor_x = factors.x * (x + 1 - start.x);

         if (x + 1 > end.x)
            factor_x = factors.x * (end.x - x);

         float factor = factor_x * factor_y;

         const int2 pos0 = { x, y };

         sum += read_imagef(source, sampler, pos0) * factor;
      }
   }

   value = sum;

   write_imagef(dest, pos, value);
}

kernel void test(__read_only image2d_t source, __write_only image2d_t outputImage)
{
// get id of element in array
   int x = get_global_id(0);
   int y = get_global_id(1);
   int w = get_global_size(0);
   int h = get_global_size(1);
   
   float4 result = (float4)(0.0f,0.0f,0.0f,1.0f);
   float MinRe = -2.0f;
   float MaxRe = 1.0f;
   float MinIm = -1.5f;
   float MaxIm = MinIm+(MaxRe-MinRe)*h/w;
   float Re_factor = (MaxRe-MinRe)/(w-1);
   float Im_factor = (MaxIm-MinIm)/(h-1);
   float MaxIterations = 250;

   //C imaginary
   float c_im = MaxIm - y*Im_factor;

   //C real
   float c_re = MinRe + x*Re_factor;

   //Z real
   float Z_re = c_re, Z_im = c_im;

   bool isInside = true;
   bool col2 = false;
   bool col3 = false;
   int iteration =0;

   for(int n=0; n<MaxIterations; n++)
   {
      // Z - real and imaginary
      float Z_re2 = Z_re*Z_re, Z_im2 = Z_im*Z_im;
            
      //if Z real squared plus Z imaginary squared is greater than c squared
      if(Z_re2 + Z_im2 > 4)
      {
         if(n >= 0 && n <= (MaxIterations/2-1))
         {
            col2 = true;
            isInside = false;
            break;
         }
         else if(n >= MaxIterations/2 && n <= MaxIterations-1)
         {
            col3 = true;
            isInside = false;
            break;
         }
      }
      Z_im = 2*Z_re*Z_im + c_im;
      Z_re = Z_re2 - Z_im2 + c_re;
      iteration++;
   }
   if(col2) 
   { 
      result = (float4)(iteration*0.05f,0.0f, 0.0f, 1.0f);
   }
   else if(col3)
   {
      result = (float4)(255, iteration*0.05f, iteration*0.05f, 1.0f);
   }
   else if(isInside)
   {
      result = (float4)(0.0f, 0.0f, 0.0f, 1.0f);
   }

   write_imagef(outputImage, (int2)(x, y), result);
}
