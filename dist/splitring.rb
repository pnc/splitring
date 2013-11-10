require 'formula'

class Splitring < Formula
  homepage 'https://github.com/pnc/splitring'
  url 'https://github.com/pnc/splitring/archive/v1.1.tar.gz'
  sha1 'b3c88326502894cb3d88e4b9b830ac6007dbc390'

  def install
    system "make"
    bin.install("splitring")
  end

  test do
    system "#{bin}/splitring --version"
  end
end
