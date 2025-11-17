let ImageSys;
let InPutButton;
let SendButton;
let FileInput;
let fileFlag;
function Even_init()
{
  /*获取html对象*/
  InPutButton = document.getElementById("Input_Button");
  SendButton = document.getElementById("Send_Button");
  FileInput = document.getElementById("File_Input");
  ImageSys = document.getElementById("Image_Input");
  /*为对象添加事件*/
  InPutButton.addEventListener("click",InPutButton_Even);
  FileInput.addEventListener("change",FileInput_Even);
  SendButton.addEventListener("click",SendButton_Even);
}
function InPutButton_Even()
{
  FileInput.click();    //触发输入文件组件的点击事件,弹出文件系统
}
function FileInput_Even(event) // change
{
  fileFlag = 0;
  file = event.target.files[0];
  if (!file) {
    alert("未选择文件");
    return;
  }

  if (!file.type.startsWith("image/")) {
    alert("请选择图片文件");
    return;
  }

  const fileName = file.name.toLowerCase();
  if (!fileName.endsWith(".bmp")) {
    alert("请选择 BMP 图片文件");
    return;
  }

  const reader = new FileReader();
  reader.onload = function (e) {
    const buffer = new Uint8Array(e.target.result);

    // BMP 文件头检查
    if (buffer[0] !== 0x42 || buffer[1] !== 0x4D) {
      alert("无效的 BMP 文件");
      return;
    }

    // 读取宽高（小端字节序）
    const width = buffer[18] | (buffer[19] << 8) | (buffer[20] << 16) | (buffer[21] << 24);
    const height = buffer[22] | (buffer[23] << 8) | (buffer[24] << 16) | (buffer[25] << 24);

    // 位深
    const bpp = buffer[28];
    if (bpp !== 24) {
      alert("请选择 24 位 BMP 文件");
      return;
    }

    // 分辨率判断
    if (!((width === 800 && height === 480) || (width === 480 && height === 800))) {
      alert(`BMP 图像分辨率必须为 800x480 或 480x800,当前为 ${width}x${height}`);
      return;
    }

    // 所有验证通过，设置图像并标记成功
    const imageURL = URL.createObjectURL(file);
    ImageSys.src = imageURL;
    ImageSys.onload = () => {
      URL.revokeObjectURL(imageURL);
    };
    fileFlag = 1;
  };

  reader.readAsArrayBuffer(file); // 读取数据 触发onload回调
}
function SendButton_Even()
{
  if(fileFlag)
  {
    const formData = new FormData();
    formData.append("file", file);              // 自动带上文件名和 Content-Type
    formData.append("user", "admin");           // 附带其他字段
    formData.append("timestamp", Date.now());

    fetch("/dataUP", {
      method: "POST",
      body: formData  // Content-Type 会自动设置为 multipart/form-data
    })
    .then(response => response.text())
    .then(result => alert("上传成功：" + result))
    .catch(error => {
      console.error("上传失败:", error);
      alert("上传失败！");
    });
  }
  else
  {
    alert("请选择图片文件");
  }
}
//DOM初始化完之后进入事件初始化
document.addEventListener("DOMContentLoaded", Even_init);//监听事件,这样就不需要在html里面实现点击回调函数




/*

function SendButton_Even()
{
  if(fileFlag)
  {
    fetch("/dataUP",{
      method:"POST",
      headers:{
          "Content-Type":file.type
      },
      body:file
      })
    .then(response => response.text())
    .then(result => {
      alert("上传成功：" + result);
    })
    .catch(error => {
      console.error("上传失败:", error);
      alert("上传失败！");
    });
  }
  else
  {
    alert("请选择图片文件");
  }
}

*/







