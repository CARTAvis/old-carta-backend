/* ************************************************************************

   Copyright:

   License:

   Authors:

************************************************************************ */

qx.Theme.define("skel.theme.Font",
{
  extend : qx.theme.modern.Font,

  fonts :
  {
      "default" :
      {
        size : 14
      },
      "titleFont" :
          {
              include : "default",
              size : 16,
              bold : true
          }
  }
});
