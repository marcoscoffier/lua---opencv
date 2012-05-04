f = { 1.1023811e-06, -0.00036657104, 0.030219628,
      0.00037386827, -3.6719496e-07, -0.39720634,
         -0.032789242, 0.39515659, 1 }

fmat = torch.Tensor(3,3)
fmat:resize(fmat:numel())
for i = 1,#f do 
   fmat[i] = f[i]
end
fmat:resize(3,3)
print("base fmat")
print(fmat)

e = { 0.40080908, -133.27968, -60.925354,
      135.93285, -0.13350652, -95.308731,
      61.810787, 96.729408, 0.70277405}

emat = torch.Tensor(3,3)
emat:resize(emat:numel())
for i = 1,#e do 
   emat[i] = e[i]
end
emat:resize(3,3)
print("base emat")
print(emat)

k = torch.Tensor(3,3):fill(0)
k[1][1] = 602 -- focal length in pixels
k[2][2] = 602 -- focal length in pixels
k[1][3] = 1280/2 -- center width
k[2][3] = 720/2 -- center height
k[3][3] = 1
print("K")
print(k)

essmat = opencv.findEssential(fmat, k)
print("my essmat")
print(essmat)